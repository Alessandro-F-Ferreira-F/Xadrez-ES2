# ---------------------------------------------------------------------------
# Xadrez-ES2 -- motor em C
#
# WARNINGS: o conjunto abaixo e o acordo do projeto (CLAUDE.md secao 7), nao
# preferencia. O build anterior usava so '-Wall -g' e por isso escondia:
#   -Wextra            -> variaveis nao usadas em generate_sliding_moves
#   -Wstrict-prototypes-> as declaracoes '()' em vez de '(void)'
#   -Wpedantic         -> o '\e' nao-ISO em clear_screen
#
# -Og E NAO -g SOZINHO: o GCC so faz analise de fluxo de dados com otimizacao
# ligada. Sem ela, -Wuninitialized e familia simplesmente nao disparam. Foi
# assim que a 'MoveList l;' sem inicializar passou batido.
# ---------------------------------------------------------------------------

CC       := gcc
STD      := -std=c11
WARN     := -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes
SRC      := $(wildcard src/*.c) test/test.c

ifeq ($(OS),Windows_NT)
BIN         := build/main.exe
MKDIR_BUILD := if not exist build mkdir build
TEST_BIN    := build/test.exe
RUN_BIN     := $(BIN)
else
BIN         := build/main.out
MKDIR_BUILD := mkdir -p build
TEST_BIN    := build/test.out
RUN_BIN     := ./$(BIN)
endif

CFLAGS   := $(STD) $(WARN) -Og -g
DBGFLAGS := $(STD) $(WARN) -Og -g -DDEBUG -fsanitize=address,undefined \
            -fno-omit-frame-pointer -Wframe-larger-than=16384

.PHONY: all debug run clean test

all:
	@$(MKDIR_BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

# -DDEBUG liga o LOG_ERROR; sem ele os erros de FEN e de lance somem.
debug:
	@$(MKDIR_BUILD)
	$(CC) $(DBGFLAGS) $(SRC) -o $(BIN)

test:
	@$(MKDIR_BUILD)
	$(CC) $(CFLAGS) -DMAKE_UNMAKE_TEST_STANDALONE \
		-o $(TEST_BIN) $(filter-out src/main.c src/io.c,$(SRC))
	$(TEST_BIN)

run: all
	$(RUN_BIN)

clean:
	rm -f build/*.out build/*.exe