DEBUG_FLAGS = -DDEBUG -Wall -Wextra -Wpedantic -g -fsanitize=address -Wframe-larger-than=16384


all:
	gcc -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

debug:
	gcc $(DEBUG_FLAGS) -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

run:
	./build/main.out

clean:
	rm -f build/*.out

context:
	@cat CLAUDE.md > context.txt
	@echo "\n\n===== INTERFACES =====\n" >> context.txt
	@for h in src/*.h; do echo "--- $$h ---"; cat $$h; echo; done >> context.txt
	@echo "context.txt: $$(wc -l < context.txt) linhas"