all: bin
	gcc -std=c99 -Wall -g *.c -ledit -lm -o ./bin/clisp

bin:
	mkdir ./bin

clean:
	rm -rf bin
