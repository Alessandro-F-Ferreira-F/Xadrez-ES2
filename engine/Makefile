DEBUG_FLAGS = -DDEBUG -Wall -Wextra -Wpedantic -g -fsanitize=address -Wframe-larger-than=16384


all:
	@mkdir -p build
	gcc -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

debug:
	@mkdir -p build
	gcc $(DEBUG_FLAGS) -Wall -g src/main.c src/board.c src/movegen.c src/log.c src/utils.c -o build/main.out

run:
	./build/main.out

clean:
	rm -f build/*.out
