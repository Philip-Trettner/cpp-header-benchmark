#include <chrono>
#include <fstream>
#include <iostream>

#include "json.hh"

int main(int argc, char const* argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: ~ test-config.json output.csv" << std::endl;

        return EXIT_SUCCESS;
    }

    using json = nlohmann::json;

    // setup
    struct compiler
    {
        std::string name;
        std::string path;
    };
    struct source
    {
        std::string name;
        std::string code;
    };
    struct flag
    {
        std::string name;
        std::vector<std::string> options;
    };

    std::vector<compiler> compilers;
    std::vector<flag> flags;
    std::vector<source> sources;

    // read config
    {
        std::cout << "reading config '" << argv[1] << "'" << std::endl;
        std::ifstream cfg_file(argv[1]);
        json cfg;
        cfg_file >> cfg;
        if (!cfg_file.good())
        {
            std::cerr << "Could not parse file " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }

        for (auto const& [name, path] : cfg["compilers"].items())
            compilers.push_back({name, path});

        for (auto const& [n, c] : cfg["flags"].items())
        {
            flag f;
            f.name = n;
            for (auto const& o : c)
                f.options.push_back(o);
            flags.push_back(f);
        }

        for (auto const& [n, c] : cfg["sources"].items())
            sources.push_back({n, c});
    }

    // initial report
    {
        std::cout << std::endl;
        std::cout << "Compilers:" << std::endl;
        for (auto const& c : compilers)
            std::cout << "  " << c.name << ": '" << c.path << "'" << std::endl;

        std::cout << std::endl;
        std::cout << "Flag Choices:" << std::endl;
        for (auto const& [name, fs] : flags)
        {
            std::cout << "  " << name << ": ";
            for (auto i = 0u; i < fs.size(); ++i)
                std::cout << (i == 0 ? "" : " | ") << "'" << fs[i] << "'";
            std::cout << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Sources:" << std::endl;
        for (auto const& s : sources)
            std::cout << "  " << s.name << ": \"" << s.code << "\"" << std::endl;
    }

    // actual execution
    {
        std::ofstream csv(argv[2]);

        // header
        csv << "source,compiler";
        for (auto const& f : flags)
            csv << "," << f.name;
        csv << ",ms";
        csv << std::endl;

        struct test
        {
            std::string csv;
            std::string cmd;
            std::string src;
        };
        std::vector<test> tests;

        for (auto const& [sn, sc] : sources)
            for (auto const& [cn, cc] : compilers)
                tests.push_back({sn + "," + cn, cc, sc});

        auto add_flag = [&](std::string name, std::vector<std::string> options) {
            std::vector<test> new_tests;
            for (auto const& t : tests)
                for (auto const& o : options)
                    new_tests.push_back({t.csv + ",\"" + o + "\"", t.cmd + " " + o, t.src});
            swap(tests, new_tests);
        };

        for (auto const& [n, f] : flags)
            add_flag(n, f);

        std::cout << std::endl;
        for (auto const& t : tests)
        {
            std::string const tmp_file = "/tmp/cpp-header-benchmark.cc";
            std::ofstream(tmp_file) << t.src << "\n";

            auto cmd = t.cmd + " -c " + tmp_file + " -o " + tmp_file + ".o";
            std::cout << std::endl;
            std::cout << "test \"" << t.csv << "\",code: \"" << t.src << "\"" << std::endl;
            std::cout << "  " << cmd << std::endl;

            auto ok = true;

            std::vector<double> times;
            for (auto i = 0; i < 3; ++i)
            {
                auto start = std::chrono::high_resolution_clock::now();
                auto res = system(cmd.c_str());
                auto end = std::chrono::high_resolution_clock::now();
                times.push_back(std::chrono::duration<double>(end - start).count());

                if (res != 0)
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
            {
                std::cout << "  ERROR while executing cmd, skipping entry" << std::endl;
                continue;
            }

            for (auto t : times)
                std::cout << "    " << t * 1000 << " ms" << std::endl;

            sort(times.begin(), times.end());

            csv << t.csv << "," << times[0] * 1000 << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
