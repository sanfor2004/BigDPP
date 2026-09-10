cmake_minimum_required(VERSION 3.25)

project(
	BigDPP
	VERSION 0.1.0
	LANGUAGES CXX
)

find_package(dpp CONFIG REQUIRED)
add_executable(bigdpp src/main.cpp)

target_compile_features(bigdpp PRIVATE cxx_std_20)
target_link_libraries(bigdpp PRIVATE dpp::dpp)


if(MSVC)
	target_compile_options(bigdpp PRIVATE /W4 /permissive-)
endif()
