cmake_minimum_required(VERSION 3.16)
project(BSConstructor VERSION 0.1.0 LANGUAGES CXX)

set(BSC_CXX_STANDARD 17 CACHE STRING "C++ standard to build with (17 or 20)")
set(CMAKE_CXX_STANDARD ${BSC_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BSC_BUILD_TESTS "Build unit tests" ON)
option(BSC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(bsc_core
    src/Types.cpp
    src/LinksStore.cpp
    src/LinkMatrix.cpp
    src/TypicalSolutions.cpp
    src/SafeString.cpp
    src/ActParser.cpp
    src/Project.cpp
    src/Validator.cpp
    src/Report.cpp
    src/BSConstructor.cpp
)
target_include_directories(bsc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    # Sources contain UTF-8 Russian literals.
    target_compile_options(bsc_core PUBLIC /utf-8 /W4 /permissive-)
    if(BSC_WARNINGS_AS_ERRORS)
        target_compile_options(bsc_core PUBLIC /WX)
    endif()
else()
    target_compile_options(bsc_core PUBLIC -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(BSC_WARNINGS_AS_ERRORS)
        target_compile_options(bsc_core PUBLIC -Werror)
    endif()
endif()

add_executable(bs_constructor app/main.cpp)
target_link_libraries(bs_constructor PRIVATE bsc_core)

if(BSC_BUILD_TESTS)
    enable_testing()
    add_executable(bsc_tests
        tests/TestRunner.cpp
        tests/test_main.cpp
        tests/test_links.cpp
        tests/test_parser.cpp
        tests/test_constructor.cpp
    )
    target_link_libraries(bsc_tests PRIVATE bsc_core)
    add_test(NAME bsc_tests COMMAND bsc_tests)
    set_tests_properties(bsc_tests PROPERTIES TIMEOUT 60)

    add_test(NAME cli_demo COMMAND bs_constructor --dwg)
    add_test(NAME cli_example_rooftop
        COMMAND bs_constructor ${CMAKE_CURRENT_SOURCE_DIR}/examples/act_t2_rooftop.txt --switch MEGAFON)
    # Switching MTS -> Megafon on a pipe stand must be reported as a KM overload
    # (exit code 2, so the check is done on the output).
    add_test(NAME cli_switch_overload
        COMMAND bs_constructor ${CMAKE_CURRENT_SOURCE_DIR}/examples/act_mts_ground.txt --switch MEGAFON)
    set_tests_properties(cli_switch_overload PROPERTIES PASS_REGULAR_EXPRESSION "E_KM_OVERLOAD")
    set_tests_properties(cli_demo cli_example_rooftop cli_switch_overload PROPERTIES TIMEOUT 30)
endif()
