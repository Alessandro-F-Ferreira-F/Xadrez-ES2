cmake_minimum_required(VERSION 3.20)
project(XadrezInterface LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra a biblioteca SFML já instalada no sistema
find_package(SFML 2 COMPONENTS graphics window system REQUIRED)

# Coleta os arquivos fonte (.cpp)
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES main.cpp)

# Define o executável
add_executable(xadrez ${SOURCES})

# Configura a pasta include/ para os headers
target_include_directories(xadrez PRIVATE include)

# Linka as bibliotecas da SFML
target_link_libraries(xadrez PRIVATE sfml-graphics sfml-window sfml-system)