cmake_minimum_required(VERSION 3.25)

project(
	BigDPP
	VERSION 0.1.0
	LANGUAGES CXX
)

find_package(dpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

enable_testing()

add_library(bigdpp_core STATIC
	src/config.cpp
)

target_compile_features(bigdpp_core PUBLIC cxx_std_20)
target_include_directories(bigdpp_core PUBLIC include)

add_executable(bigdpp
	src/admin.cpp
	src/bot.cpp
	src/llm.cpp
	src/main.cpp
)

target_compile_features(bigdpp PRIVATE cxx_std_20)
target_include_directories(bigdpp PRIVATE include)
target_link_libraries(bigdpp PRIVATE bigdpp_core dpp::dpp nlohmann_json::nlohmann_json)

add_executable(bigdpp_tests
	tests/config_tests.cpp
)

target_compile_features(bigdpp_tests PRIVATE cxx_std_20)
target_link_libraries(bigdpp_tests PRIVATE bigdpp_core)
add_test(NAME bigdpp_configuration COMMAND bigdpp_tests)

add_executable(bigdpp_admin_tests
	src/admin.cpp
	tests/admin_tests.cpp
)

target_compile_features(bigdpp_admin_tests PRIVATE cxx_std_20)
target_include_directories(bigdpp_admin_tests PRIVATE include)
target_link_libraries(bigdpp_admin_tests PRIVATE dpp::dpp)
add_test(NAME bigdpp_administration COMMAND bigdpp_admin_tests)


if(MSVC)
	target_compile_options(bigdpp PRIVATE /W4 /permissive-)
	target_compile_options(bigdpp_core PRIVATE /W4 /permissive-)
	target_compile_options(bigdpp_tests PRIVATE /W4 /permissive-)
	target_compile_options(bigdpp_admin_tests PRIVATE /W4 /permissive-)
else()
	target_compile_options(bigdpp PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(bigdpp_core PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(bigdpp_tests PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(bigdpp_admin_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
