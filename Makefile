all: bin
	gcc -std=c99 -Wall -g *.c -ledit -o ./bin/clisp

bin:
	mkdir ./bin

clean:
	rm -rf bin
