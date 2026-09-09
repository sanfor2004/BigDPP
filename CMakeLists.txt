cmake_minimum_required(VERSION 3.25)

project(
    BigDPP
    VERSION 0.1.0
    DESCRIPTION "A C++ Discord operations bot"
    LANGUAGES CXX
)

option(BIGDPP_BUILD_TESTS "Build BigDPP tests" ON)

find_package(spdlog CONFIG REQUIRED)

add_library(bigdpp_core
    src/config/config.cpp
    src/logging/logging.cpp
)
add_library(bigdpp::core ALIAS bigdpp_core)

target_compile_features(bigdpp_core PUBLIC cxx_std_20)
target_include_directories(
    bigdpp_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(bigdpp_core PRIVATE spdlog::spdlog)

if(MSVC)
    target_compile_options(bigdpp_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(bigdpp_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bigdpp src/main.cpp)
target_compile_features(bigdpp PRIVATE cxx_std_20)
target_link_libraries(bigdpp PRIVATE bigdpp::core spdlog::spdlog)
target_compile_definitions(bigdpp PRIVATE BIGDPP_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(bigdpp PRIVATE /W4 /permissive-)
else()
    target_compile_options(bigdpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BIGDPP_BUILD_TESTS)
    include(CTest)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(bigdpp_tests tests/config_tests.cpp)
    target_compile_features(bigdpp_tests PRIVATE cxx_std_20)
    target_link_libraries(bigdpp_tests PRIVATE bigdpp::core Catch2::Catch2WithMain)

    if(MSVC)
        target_compile_options(bigdpp_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(bigdpp_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(Catch)
    catch_discover_tests(bigdpp_tests)
endif()
