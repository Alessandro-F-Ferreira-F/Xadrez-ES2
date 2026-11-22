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

# Biblioteca padronizada de testes (tools/ctest na raiz do repositorio)
SRC_CTEST  := ../tools/ctest/test_api.c ../tools/ctest/timer.c

# Nucleo do backend sem o main.c do motor (para linkar nos binarios de teste)
SRC_CORE    := $(filter-out $(ENGINE)/src/main.c,$(SRC_ENGINE)) $(SRC_IA)
SRC_IA_TEST := $(filter-out $(ENGINE)/src/main.c,$(SRC_ENGINE)) $(addprefix $(IA)/src/,ia.c avaliar.c ordenar.c niveis.c stubs.c)

# Testes da engine: rules.c, search.c e test.c
SRC_TEST   := $(wildcard $(ENGINE)/test/*.c) $(SRC_CTEST) $(SRC_CORE)

HEADERS    := $(wildcard $(ENGINE)/include/*.h) $(wildcard $(IA)/header/*.h)

ifeq ($(OS),Windows_NT)
# Forca o cmd.exe: sem isso o mingw32-make usa o sh.exe do Git se ele estiver
# no PATH, e os comandos abaixo deixariam de valer.
SHELL       := cmd.exe
EXE         := .exe
MKDIR_BUILD := if not exist $(subst /,\,$(BUILD)) mkdir $(subst /,\,$(BUILD))
RM_BUILD    := del /q $(subst /,\,$(BUILD))\main.exe $(subst /,\,$(BUILD))\test.exe $(subst /,\,$(BUILD))\ia_unit_test.exe $(subst /,\,$(BUILD))\ia_integration_test.exe $(subst /,\,$(BUILD))\ia_system_test.exe 2>nul
PYTHON      := python
else
EXE         := .out
MKDIR_BUILD := mkdir -p $(BUILD)
RM_BUILD    := rm -f $(BUILD)/main.out $(BUILD)/test.out $(BUILD)/ia_unit_test.out $(BUILD)/ia_integration_test.out $(BUILD)/ia_system_test.out
PYTHON      := python3
endif

BIN              := $(BUILD)/main$(EXE)
TEST_BIN         := $(BUILD)/test$(EXE)
IA_UNIT_BIN      := $(BUILD)/ia_unit_test$(EXE)
IA_INTEG_BIN     := $(BUILD)/ia_integration_test$(EXE)
IA_SYSTEM_BIN    := $(BUILD)/ia_system_test$(EXE)
COV_DIR          := $(BUILD)/cov
COV_ENGINE_OBJS  := $(patsubst $(ENGINE)/src/%.c,$(COV_DIR)/%.o,$(SRC_ENGINE))
COV_IA_OBJS      := $(patsubst $(IA)/src/%.c,$(COV_DIR)/%.o,$(SRC_IA))

.PHONY: all run test test-engine test-ia test-ia-unit test-ia-integration test-ia-system protocol coverage clean

all: $(BIN)

$(BIN): $(SRC_ENGINE) $(SRC_IA) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_ENGINE) $(SRC_IA) -o $@

$(TEST_BIN): $(SRC_TEST) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) -DMAKE_UNMAKE_TEST_STANDALONE -o $@

$(IA_UNIT_BIN): $(IA)/test/unit_test.c $(IA)/test/ia_test_common.h $(SRC_CTEST) $(SRC_IA_TEST) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(IA)/test/unit_test.c $(SRC_CTEST) $(SRC_IA_TEST) -o $@

$(IA_INTEG_BIN): $(IA)/test/integration_test.c $(IA)/test/ia_test_common.h $(SRC_CTEST) $(SRC_IA_TEST) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(IA)/test/integration_test.c $(SRC_CTEST) $(SRC_IA_TEST) -o $@

$(IA_SYSTEM_BIN): $(IA)/test/system_test.c $(IA)/test/ia_test_common.h $(SRC_CTEST) $(SRC_IA_TEST) $(HEADERS)
	@$(MKDIR_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(IA)/test/system_test.c $(SRC_CTEST) $(SRC_IA_TEST) -o $@

# PERFT_MAX_DEPTH e TEST_FILTER sao lidos do ambiente pelo proprio test.out.
test-engine: $(TEST_BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(TEST_BIN))

test-ia-unit: $(IA_UNIT_BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(IA_UNIT_BIN))

test-ia-integration: $(IA_INTEG_BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(IA_INTEG_BIN))

test-ia-system: $(IA_SYSTEM_BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(IA_SYSTEM_BIN))

test-ia: test-ia-unit test-ia-integration test-ia-system

test: test-engine test-ia

run: $(BIN)
	$(subst /,$(if $(filter Windows_NT,$(OS)),\,/),$(BIN))

protocol: $(BIN)
	$(PYTHON) $(ENGINE)/test/protocol/run.py $(BIN)

coverage:
	@mkdir -p $(COV_DIR)
	@rm -f $(COV_DIR)/*.o $(COV_DIR)/*.gcda $(COV_DIR)/*.gcno $(COV_DIR)/*.gcov *.gcov
	@for f in $(SRC_ENGINE); do \
		obj="$(COV_DIR)/$$(basename $$f .c).o"; \
		$(CC) $(CPPFLAGS) $(CFLAGS) --coverage -c $$f -o $$obj || exit 1; \
	done
	@for f in $(SRC_IA); do \
		obj="$(COV_DIR)/$$(basename $$f .c).o"; \
		$(CC) $(CPPFLAGS) $(CFLAGS) --coverage -c $$f -o $$obj || exit 1; \
	done
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(IA)/src/stubs.c -o $(COV_DIR)/stubs.o
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(COV_ENGINE_OBJS) $(COV_IA_OBJS) -o $(COV_DIR)/main$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(wildcard $(ENGINE)/test/*.c) $(SRC_CTEST) $(filter-out $(COV_DIR)/main.o,$(COV_ENGINE_OBJS)) $(COV_IA_OBJS) -DMAKE_UNMAKE_TEST_STANDALONE -o $(COV_DIR)/test$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(IA)/test/unit_test.c $(SRC_CTEST) $(filter-out $(COV_DIR)/main.o,$(COV_ENGINE_OBJS)) $(filter-out $(COV_DIR)/regras.o,$(COV_IA_OBJS)) $(COV_DIR)/stubs.o -o $(COV_DIR)/ia_unit_test$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(IA)/test/integration_test.c $(SRC_CTEST) $(filter-out $(COV_DIR)/main.o,$(COV_ENGINE_OBJS)) $(filter-out $(COV_DIR)/regras.o,$(COV_IA_OBJS)) $(COV_DIR)/stubs.o -o $(COV_DIR)/ia_integration_test$(EXE)
	$(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(IA)/test/system_test.c $(SRC_CTEST) $(filter-out $(COV_DIR)/main.o,$(COV_ENGINE_OBJS)) $(filter-out $(COV_DIR)/regras.o,$(COV_IA_OBJS)) $(COV_DIR)/stubs.o -o $(COV_DIR)/ia_system_test$(EXE)
	./$(COV_DIR)/test$(EXE)
	./$(COV_DIR)/ia_unit_test$(EXE)
	./$(COV_DIR)/ia_integration_test$(EXE)
	./$(COV_DIR)/ia_system_test$(EXE)
	-$(PYTHON) $(ENGINE)/test/protocol/run.py $(COV_DIR)/main$(EXE)
	gcov -r -o $(COV_DIR) $(SRC_ENGINE) $(SRC_IA)
	@mv -f *.gcov $(COV_DIR)/ 2>/dev/null || true

clean:
	-$(RM_BUILD)
	-rm -rf $(COV_DIR) *.gcov
