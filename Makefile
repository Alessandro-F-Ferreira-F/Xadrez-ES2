# ---------------------------------------------------------------------------
# Xadrez-ES2 -- Makefile Global (Backend: Engine + IA | Interface SFML 2)
#
# Comandos principais a partir da raiz do repositorio:
#   make               compila Backend (Backend/build/main.out) e Interface (interface/build/xadrez)
#   make run           compila Backend + Interface e inicia o jogo grafico
#   make test          roda todos os testes (engine com perft d3, IA, protocolo UCI e interface)
#   make test-full     roda todos os testes com perft completo (profundidade 5)
#   make test-backend  roda testes C do Backend (engine + IA)
#   make test-engine   roda apenas a suite de regras/busca da engine
#   make test-ia       roda apenas as suites de unidade, integracao e sistema da IA
#   make test-protocol roda a suite Python de protocolo UCI
#   make test-interface roda as suites de smoke, unidade, integracao e sistema da interface
#   make coverage      gera cobertura (gcov) de todos os modulos e exibe resumo consolidado
#   make coverage-backend   gera cobertura (gcov) de Backend/engine e Backend/IA
#   make coverage-interface gera cobertura (gcov) da interface
#   make clean         remove binarios de Backend/build e interface/build
# ---------------------------------------------------------------------------

PERFT_MAX_DEPTH ?= 3

.PHONY: all backend interface run test test-full test-backend test-engine test-ia test-protocol test-interface coverage coverage-backend coverage-interface clean

all: backend interface

backend:
	$(MAKE) -C Backend

interface:
	$(MAKE) -C interface

run: backend
	$(MAKE) -C interface run

test-engine:
	PERFT_MAX_DEPTH=$(PERFT_MAX_DEPTH) $(MAKE) -C Backend test-engine

test-ia:
	$(MAKE) -C Backend test-ia

test-backend:
	PERFT_MAX_DEPTH=$(PERFT_MAX_DEPTH) $(MAKE) -C Backend test

test-protocol:
	$(MAKE) -C Backend protocol

test-interface: backend
	$(MAKE) -C interface test

test: test-backend test-protocol test-interface

test-full:
	$(MAKE) test PERFT_MAX_DEPTH=5

coverage-backend:
	PERFT_MAX_DEPTH=$(PERFT_MAX_DEPTH) $(MAKE) -C Backend coverage

coverage-interface: backend
	$(MAKE) -C interface coverage

coverage: coverage-backend coverage-interface
	@./tools/coverage_report.sh

clean:
	$(MAKE) -C Backend clean
	$(MAKE) -C interface clean
