#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

import tube;

TEST_CASE("Core command-line parsing", "[benchmark]")
{
    char program[]{"tek4010"};
    char baudOption[]{"-b9600"};
    char waitOption[]{"-wait"};
    char waitValue[]{"12"};
    char command[]{"shell"};
    char* arguments[]{program, baudOption, waitOption, waitValue, command};

    BENCHMARK("parse representative startup arguments")
    {
        TubeCoreOptions options;
        return options.parseCommandLine(5, arguments);
    };
}

TEST_CASE("Core key translation", "[benchmark]")
{
    TubeInputState input;
    REQUIRE(input.addKeyTranslation(0, 'a', 'b'));

    BENCHMARK("translate a mapped key")
    {
        return input.translateKeyCode('a');
    };

    BENCHMARK("pass through an unmapped key")
    {
        return input.translateKeyCode('z');
    };
}

TEST_CASE("Core coordinate updates", "[benchmark]")
{
    BENCHMARK("apply 256 coordinate updates and synchronize the target")
    {
        TubeCoordinates coordinates;
        for (int update{}; update < 256; ++update)
        {
            coordinates.moveCurrentX(1);
            coordinates.moveCurrentY(1);
            coordinates.setTargetToCurrent();
            coordinates.copyTargetToCurrent();
        }
        return coordinates.currentX() + coordinates.currentY();
    };
}
