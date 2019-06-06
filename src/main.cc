#include <chrono>
#include <fstream>
#include <iostream>

#include "json.hh"

int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: ~ test-config.json" << std::endl;

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

    std::vector<compiler> compilers;
    std::map<std::string, std::vector<std::string>> flags;
    std::vector<source> sources;

    // read config
    {
        std::cerr << "reading config '" << argv[1] << "'" << std::endl;
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
            std::vector<std::string> fs;
            for (auto const& f : c)
                fs.push_back(f);
            flags[n] = fs;
        }

        for (auto const& [n, c] : cfg["sources"].items())
            sources.push_back({n, c});
    }

    // initial report
    {
        std::cerr << std::endl;
        std::cerr << "Compilers:" << std::endl;
        for (auto const& c : compilers)
            std::cerr << "  " << c.name << ": '" << c.path << "'" << std::endl;

        std::cerr << std::endl;
        std::cerr << "Flag Choices:" << std::endl;
        for (auto const& [name, fs] : flags)
        {
            std::cerr << "  " << name << ": ";
            for (auto i = 0u; i < fs.size(); ++i)
                std::cerr << (i == 0 ? "" : " | ") << "'" << fs[i] << "'";
            std::cerr << std::endl;
        }

        std::cerr << std::endl;
        std::cerr << "Sources:" << std::endl;
        for (auto const& s : sources)
            std::cerr << "  " << s.name << ": \"" << s.code << "\"" << std::endl;
    }

    // actual execution
    {
        // header
        std::cout << "source, compiler";
        for (auto const& f : flags)
            std::cout << ", " << f.first;
        std::cout << ", ms";
        std::cout << std::endl;

        struct test
        {
            std::string csv;
            std::string cmd;
            std::string src;
        };
        std::vector<test> tests;

        for (auto const& [sn, sc] : sources)
            for (auto const& [cn, cc] : compilers)
                tests.push_back({sn + ", " + cn, cc, sc});

        auto add_flag = [&](std::string name, std::vector<std::string> options) {
            std::vector<test> new_tests;
            for (auto const& t : tests)
                for (auto const& o : options)
                    new_tests.push_back({t.csv + ", '" + o + "'", t.cmd + " " + o, t.src});
            swap(tests, new_tests);
        };

        for (auto const& [n, f] : flags)
            add_flag(n, f);

        std::cerr << std::endl;
        for (auto const& t : tests)
        {
            std::string const tmp_file = "/tmp/cpp-header-benchmark.cc";
            std::ofstream(tmp_file) << t.src << "\n";

            auto cmd = t.cmd + " -c " + tmp_file + " -o " + tmp_file + ".o";
            std::cerr << "test \"" << t.csv << "\", code: \"" << t.src << "\"" << std::endl;
            std::cerr << "  " << cmd << std::endl;

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
                std::cerr << "  ERROR while executing cmd, skipping entry" << std::endl;
                continue;
            }

            // for (auto t : times)
            //     std::cerr << "    " << t * 1000 << " ms" << std::endl;

            sort(times.begin(), times.end());

            std::cout << t.csv << ", " << times[0] * 1000 << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
