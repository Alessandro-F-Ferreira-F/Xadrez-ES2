DEBUG_FLAGS = -DDEBUG -Wall -Wextra -Wpedantic -g -fsanitize=address -Wframe-larger-than=16384


all:
	gcc -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

debug:
	gcc $(DEBUG_FLAGS) -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

run:
	./build/main.out

clean:
	rm -f build/*.out

# Monta o pacote de contexto para colar numa conversa (Claude web, etc).
# docs/project_context.md e' escrito a mao -- este alvo so junta as pecas.
context:
	@mkdir -p build
	@cat CLAUDE.md docs/project_context.md > build/context.md
	@echo "build/context.md: $$(wc -l < build/context.md) linhas, $$(wc -c < build/context.md) bytes"