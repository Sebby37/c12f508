all: main.c src/*.c include/*.h
	gcc -Iinclude main.c src/*.c

tests: test_sleepled

test_sleepled: tests/test_sleepled.c src/*.c include/*.h
	gcc -Iinclude tests/test_sleepled.c src/*.c

clean:
	rm a.out