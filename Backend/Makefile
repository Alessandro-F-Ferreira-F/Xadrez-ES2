# ---------------------------------------------------------------------------
# Xadrez-ES2 -- Backend: maquina de regras (engine/) + IA (IA/)
#
# Rode a partir de Backend/ (ou com 'make -C Backend' da raiz do repositorio).
#
#   make            compila motor + IA  -> build/main.out (main.exe no Windows)
#   make test       compila e roda a suite C -> build/test.out
#   make run        compila e abre o motor (protocolo UCI na stdin/stdout)
#   make protocol   compila e roda os testes de protocolo (Python)
#   make clean      apaga os binarios gerados
#
# Os binarios ficam em Backend/build/; 'make protocol' passa o caminho do
# motor explicitamente ao engine/test/protocol/run.py.
#
# WARNINGS: o conjunto abaixo e o acordo do projeto. -Og (e nao so -g) porque
# o GCC so faz a analise de fluxo que alimenta -Wuninitialized com otimizacao.
# ---------------------------------------------------------------------------

CC       := gcc
STD      := -std=c11
WARN     := -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes
CFLAGS   := $(STD) $(WARN) -Og -g

ENGINE   := engine
IA       := IA
BUILD    := build

CPPFLAGS := -I$(ENGINE)/include -I$(IA)/header

# Motor: todos os .c de engine/src.
SRC_ENGINE := $(wildcard $(ENGINE)/src/*.c)

# IA: lista explicita. stubs.c (regras falsas) e teste.c (main proprio) ficam
# de fora; regras.c e o adaptador que liga a IA as regras reais do motor.
SRC_IA     := $(addprefix $(IA)/src/,ia.c avaliar.c ordenar.c niveis.c regras.c)

# Testes: tudo menos o main.c do motor (a suite tem main proprio).
SRC_TEST   := $(wildcard $(ENGINE)/test/*.c) $(wildcard $(ENGINE)/test/ctest/*.c) \
              $(filter-out $(ENGINE)/src/main.c,$(SRC_ENGINE)) $(SRC_IA)

HEADERS    := $(wildcard $(ENGINE)/include/*.h) $(wildcard $(IA)/header/*.h)

ifeq ($(OS),Windows_NT)
# Forca o cmd.exe: sem isso o mingw32-make usa o sh.exe do Git se ele estiver
# no PATH, e os comandos abaixo deixariam de valer.
SHELL       := cmd.exe
EXE         := .exe
MKDIR_BUILD := if not exist $(subst /,\,$(BUILD)) mkdir $(subst /,\,$(BUILD))
RM_BUILD    := del /q $(subst /,\,$(BUILD))\main.exe $(subst /,\,$(BUILD))\test.exe 2>nul
PYTHON      := python
else
EXE         := .out
MKDIR_BUILD := mkdir -p $(BUILD)
RM_BUILD    := rm -f $(BUILD)/main.out $(BUILD)/test.out
PYTHON      := python3
endif

BIN      := $(BUILD)/main$(EXE)
TEST_BIN := $(BUILD)/test$(EXE)

.PHONY: all run test protocol clean

all: $(BIN)

$(BIN): $(SRC_ENGINE) $(SRC_IA) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_ENGINE) $(SRC_IA) -o $@

$(TEST_BIN): $(SRC_TEST) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) -DMAKE_UNMAKE_TEST_STANDALONE -o $@

# PERFT_MAX_DEPTH e TEST_FILTER sao lidos do ambiente pelo proprio test.out.
test: $(TEST_BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(TEST_BIN))

run: $(BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(BIN))

protocol: $(BIN)
	$(PYTHON) $(ENGINE)/test/protocol/run.py $(abspath $(BIN))

clean:
	-$(RM_BUILD)
