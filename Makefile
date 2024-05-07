all: main.o functions.o
	gcc -o shell main.o functions.o
main.o: main.c
	gcc -c main.c
functions.o: functions.h functions.c
	gcc -c functions.c functions.h
clean:
	rm -rf *.o shell