cmake_minimum_required(VERSION 3.20)
project(XadrezInterface LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES main.cpp)
file(GLOB_RECURSE CORE_SOURCES "src/*.cpp")

# Define o executável
add_executable(xadrez ${SOURCES})

# Configura a pasta include/ para os headers
target_include_directories(xadrez PRIVATE include)

# No caso de compilação estática, define SFML_STATIC e vincula os runtimes estaticamente
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(xadrez PRIVATE SFML_STATIC)
    if(MINGW)
        target_link_options(xadrez PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

# Linka as bibliotecas da SFML (alvo clássico ou NAMESPACE)
if(TARGET SFML::Graphics)
    target_link_libraries(xadrez PRIVATE SFML::Graphics SFML::Window SFML::System)
else()
    target_link_libraries(xadrez PRIVATE sfml-graphics sfml-window sfml-system)
endif()

enable_testing()

file(GLOB TEST_SOURCES "test/*_test.cpp")
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${CORE_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE include)
    if(TARGET SFML::Graphics)
        target_link_libraries(${TEST_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)
    else()
        target_link_libraries(${TEST_NAME} PRIVATE sfml-graphics sfml-window sfml-system)
    endif()
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endforeach()