#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace
{

using namespace std::chrono_literals;

constexpr auto PLT_RENDER_TIMEOUT{30s};
constexpr auto CHILD_TERMINATION_GRACE{2s};
constexpr auto CHILD_POLL_INTERVAL{10ms};

bool waitUntilChildExits(pid_t child, int& status,
                         std::chrono::steady_clock::time_point deadline)
{
    while (true)
    {
        pid_t waited{};
        do
        {
            waited = waitpid(child, &status, WNOHANG);
        } while ((waited == -1) && (errno == EINTR));

        if (waited == child)
        {
            return true;
        }
        if (waited == -1)
        {
            throw std::system_error{errno, std::generic_category(),
                                    "waitpid failed"};
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(CHILD_POLL_INTERVAL);
    }
}

void terminateAndReapChild(pid_t child, int& status)
{
    if ((kill(child, SIGTERM) == -1) && (errno != ESRCH))
    {
        throw std::system_error{errno, std::generic_category(),
                                "Cannot terminate timed-out PLT renderer"};
    }

    if (waitUntilChildExits(child, status,
                            std::chrono::steady_clock::now() +
                                CHILD_TERMINATION_GRACE))
    {
        return;
    }

    if ((kill(child, SIGKILL) == -1) && (errno != ESRCH))
    {
        throw std::system_error{errno, std::generic_category(),
                                "Cannot force-stop timed-out PLT renderer"};
    }

    pid_t waited{};
    do
    {
        waited = waitpid(child, &status, 0);
    } while ((waited == -1) && (errno == EINTR));
    if (waited == -1)
    {
        throw std::system_error{errno, std::generic_category(),
                                "Cannot reap timed-out PLT renderer"};
    }
}

int renderPltFile(const char* relativePath)
{
    const std::filesystem::path plotPath{
        std::filesystem::path{TEK4010_SOURCE_DIR} / relativePath};
    if (!std::filesystem::is_regular_file(plotPath))
    {
        throw std::runtime_error{"PLT benchmark file not found: " +
                                 plotPath.string()};
    }

    std::string appPath{TEK4010_GTK3_APP_PATH};
    std::string plotPathString{plotPath.string()};
    char performanceOption[]{"-performance"};
    char exitOption[]{"-exit"};
    char catCommand[]{"cat"};
    char* const arguments[]{appPath.data(), performanceOption, exitOption,
                            catCommand, plotPathString.data(), nullptr};

    const pid_t child{fork()};
    if (child == -1)
    {
        throw std::system_error{errno, std::generic_category(), "fork failed"};
    }

    if (child == 0)
    {
        const int nullDescriptor{open("/dev/null", O_WRONLY)};
        if ((nullDescriptor == -1) ||
            (dup2(nullDescriptor, STDOUT_FILENO) == -1) ||
            (dup2(nullDescriptor, STDERR_FILENO) == -1))
        {
            _exit(126);
        }
        close(nullDescriptor);
        execv(appPath.c_str(), arguments);
        _exit(127);
    }

    int status{};
    if (!waitUntilChildExits(child, status,
                             std::chrono::steady_clock::now() +
                                 PLT_RENDER_TIMEOUT))
    {
        terminateAndReapChild(child, status);
        throw std::runtime_error{
            "GTK3 PLT rendering timed out after " +
            std::to_string(PLT_RENDER_TIMEOUT.count()) + " seconds for " +
            plotPath.string()};
    }
    if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
    {
        throw std::runtime_error{"GTK3 PLT rendering failed for " +
                                 plotPath.string()};
    }

    return 1;
}

} // namespace

#define PLT_RENDER_BENCHMARK(name, path)                                       \
    TEST_CASE(name, "[benchmark][plt]")                                        \
    {                                                                          \
        BENCHMARK(name)                                                        \
        {                                                                      \
            return renderPltFile(path);                                        \
        };                                                                     \
    }

PLT_RENDER_BENCHMARK("Draw test.plt with GTK/Cairo", "test.plt")
PLT_RENDER_BENCHMARK("Draw animation.plt with GTK/Cairo", "animation.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/01_barchart.plt with GTK/Cairo",
                     "pltfiles/01_barchart.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/02-das-dot.plt with GTK/Cairo",
                     "pltfiles/02-das-dot.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/03_pretty016.plt with GTK/Cairo",
                     "pltfiles/03_pretty016.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/04_slinky.plt with GTK/Cairo",
                     "pltfiles/04_slinky.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/05_snoopy.plt with GTK/Cairo",
                     "pltfiles/05_snoopy.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/06_ssws.plt with GTK/Cairo",
                     "pltfiles/06_ssws.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/07_usmap.plt with GTK/Cairo",
                     "pltfiles/07_usmap.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/08_isco36.plt with GTK/Cairo",
                     "pltfiles/08_isco36.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/09_wizard.plt with GTK/Cairo",
                     "pltfiles/09_wizard.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/10_wizidd.plt with GTK/Cairo",
                     "pltfiles/10_wizidd.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/11_wizwarp.plt with GTK/Cairo",
                     "pltfiles/11_wizwarp.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/12-ekg4.plt with GTK/Cairo",
                     "pltfiles/12-ekg4.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/13_bugss14.plt with GTK/Cairo",
                     "pltfiles/13_bugss14.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/14_fudge.plt with GTK/Cairo",
                     "pltfiles/14_fudge.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/15_bostonufo.plt with GTK/Cairo",
                     "pltfiles/15_bostonufo.plt")
PLT_RENDER_BENCHMARK("Draw pltfiles/16_gplanet.plt with GTK/Cairo",
                     "pltfiles/16_gplanet.plt")

#undef PLT_RENDER_BENCHMARK
