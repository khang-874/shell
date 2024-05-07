all: main.o
	gcc -o shell main.o
main.o: main.c
	gcc -c main.c
clean:
	rm -rf *.o shell