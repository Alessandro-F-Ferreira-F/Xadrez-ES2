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
SRC      := $(wildcard src/*.c)
BIN      := build/main.out

CFLAGS   := $(STD) $(WARN) -Og -g
DBGFLAGS := $(STD) $(WARN) -Og -g -DDEBUG -fsanitize=address,undefined \
            -fno-omit-frame-pointer -Wframe-larger-than=16384

.PHONY: all debug run clean

all:
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

# -DDEBUG liga o LOG_ERROR; sem ele os erros de FEN e de lance somem.
debug:
	@mkdir -p build
	$(CC) $(DBGFLAGS) $(SRC) -o $(BIN)

run: all
	./$(BIN)

clean:
	rm -f build/*.out
