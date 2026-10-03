CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
LIBS     := -lsfml-graphics -lsfml-window -lsfml-system

SRC      := main.cpp $(wildcard src/*.cpp)
BIN_DIR  := build
BIN      := $(BIN_DIR)/xadrez

.PHONY: all run clean

all:
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN) $(LIBS)

run: all
	./$(BIN)

clean:
	rm -rf $(BIN_DIR)
