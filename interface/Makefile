CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
LIBS     := -lsfml-graphics -lsfml-window -lsfml-system -pthread

SRC          := main.cpp $(wildcard src/*.cpp)
CORE_SRC     := $(wildcard src/*.cpp)
BIN_DIR      := build
BIN          := $(BIN_DIR)/xadrez
SMOKE_BIN    := $(BIN_DIR)/smoke_test
UNIT_BIN     := $(BIN_DIR)/unit_test
INTEG_BIN    := $(BIN_DIR)/integration_test
SYSTEM_BIN   := $(BIN_DIR)/system_test
COV_DIR      := $(BIN_DIR)/cov
CTEST_OBJS   := $(BIN_DIR)/test_api.o $(BIN_DIR)/timer.o

.PHONY: all run test test-smoke test-unit test-integration test-system coverage profile clean

all:
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN) $(LIBS)

run: all
	./$(BIN)

$(BIN_DIR)/test_api.o: ../tools/ctest/test_api.c ../tools/ctest/test_api.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c ../tools/ctest/test_api.c -o $(BIN_DIR)/test_api.o

$(BIN_DIR)/timer.o: ../tools/ctest/timer.c ../tools/ctest/test_api.h
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c ../tools/ctest/timer.c -o $(BIN_DIR)/timer.o

test-smoke: $(CTEST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) test/smoke_test.cpp $(CORE_SRC) $(CTEST_OBJS) -o $(SMOKE_BIN) $(LIBS)
	./$(SMOKE_BIN)

test-unit: $(CTEST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) test/unit_test.cpp $(CORE_SRC) $(CTEST_OBJS) -o $(UNIT_BIN) $(LIBS)
	./$(UNIT_BIN)

test-integration: $(CTEST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) test/integration_test.cpp $(CORE_SRC) $(CTEST_OBJS) -o $(INTEG_BIN) $(LIBS)
	./$(INTEG_BIN)

test-system: $(CTEST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) test/system_test.cpp $(CORE_SRC) $(CTEST_OBJS) -o $(SYSTEM_BIN) $(LIBS)
	./$(SYSTEM_BIN)

test: test-smoke test-unit test-integration test-system

coverage: $(CTEST_OBJS)
	@mkdir -p $(COV_DIR)
	rm -f $(COV_DIR)/*.gcda $(COV_DIR)/*.gcno $(COV_DIR)/*.gcov *.gcov
	$(CXX) $(CXXFLAGS) --coverage -c src/piece.cpp -o $(COV_DIR)/piece.o
	$(CXX) $(CXXFLAGS) --coverage -c src/board.cpp -o $(COV_DIR)/board.o
	$(CXX) $(CXXFLAGS) --coverage -c src/hud_panel.cpp -o $(COV_DIR)/hud_panel.o
	$(CXX) $(CXXFLAGS) --coverage -c src/engine_bridge.cpp -o $(COV_DIR)/engine_bridge.o
	$(CXX) $(CXXFLAGS) --coverage test/unit_test.cpp $(COV_DIR)/piece.o $(COV_DIR)/board.o $(COV_DIR)/hud_panel.o $(COV_DIR)/engine_bridge.o $(CTEST_OBJS) -o $(COV_DIR)/unit_test $(LIBS)
	$(CXX) $(CXXFLAGS) --coverage test/integration_test.cpp $(COV_DIR)/piece.o $(COV_DIR)/board.o $(COV_DIR)/hud_panel.o $(COV_DIR)/engine_bridge.o $(CTEST_OBJS) -o $(COV_DIR)/integration_test $(LIBS)
	$(CXX) $(CXXFLAGS) --coverage test/system_test.cpp $(COV_DIR)/piece.o $(COV_DIR)/board.o $(COV_DIR)/hud_panel.o $(COV_DIR)/engine_bridge.o $(CTEST_OBJS) -o $(COV_DIR)/system_test $(LIBS)
	./$(COV_DIR)/unit_test
	./$(COV_DIR)/integration_test
	./$(COV_DIR)/system_test
	gcov -o $(COV_DIR) src/piece.cpp src/board.cpp src/hud_panel.cpp src/engine_bridge.cpp
	@mv -f *.gcov $(COV_DIR)/ 2>/dev/null || true

profile: test-system
	valgrind --leak-check=full --error-exitcode=1 ./$(SYSTEM_BIN)

clean:
	rm -rf $(BIN_DIR) *.gcov
