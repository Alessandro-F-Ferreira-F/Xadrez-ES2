cmake_minimum_required(VERSION 3.20)
project(XadrezInterface LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_TESTING "Compila as suites de teste da interface" OFF)

# Tenta encontrar a SFML 2 instalada no sistema; se não houver, baixa e compila automaticamente via FetchContent
find_package(SFML 2 COMPONENTS graphics window system QUIET)

if(NOT SFML_FOUND)
    set(BUILD_SHARED_LIBS FALSE CACHE BOOL "Build static libraries" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
    FetchContent_MakeAvailable(SFML)
endif()

# Coleta os arquivos fonte (.cpp)
file(GLOB_RECURSE CORE_SOURCES "src/*.cpp")

find_package(Threads REQUIRED)

function(configure_interface_target target_name)
    target_include_directories(${target_name} PRIVATE include)
    if(NOT BUILD_SHARED_LIBS)
        target_compile_definitions(${target_name} PRIVATE SFML_STATIC)
        if(MINGW)
            target_link_options(${target_name} PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
    if(TARGET SFML::Graphics)
        target_link_libraries(${target_name} PRIVATE SFML::Graphics SFML::Window SFML::System Threads::Threads)
    else()
        target_link_libraries(${target_name} PRIVATE sfml-graphics sfml-window sfml-system Threads::Threads)
    endif()
endfunction()

# Define o executável principal
add_executable(xadrez ${CORE_SOURCES} main.cpp)
configure_interface_target(xadrez)

if(BUILD_TESTING)
    set(CTEST_SOURCES
        ../tools/ctest/test_api.c
        ../tools/ctest/timer.c
    )
    function(add_interface_test target_name test_src)
        add_executable(${target_name} ${test_src} ${CORE_SOURCES} ${CTEST_SOURCES})
        configure_interface_target(${target_name})
    endfunction()

    add_interface_test(smoke_test test/smoke_test.cpp)
    add_interface_test(unit_test test/unit_test.cpp)
    add_interface_test(integration_test test/integration_test.cpp)
    add_interface_test(system_test test/system_test.cpp)
endif()