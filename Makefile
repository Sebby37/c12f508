all: main.c src/*.c include/*.h
	gcc -Iinclude main.c src/*.c -o main

tests: test_sleepled test_divide

test_sleepled: tests/test_sleepled.c src/*.c include/*.h
	gcc -Iinclude tests/test_sleepled.c src/*.c -o tests/test_sleepled

test_divide: tests/test_divide.c src/*.c include/*.h
	gcc -Iinclude tests/test_divide.c src/*.c -o tests/test_divide

clean:
	rm a.out