CC = gcc
CFLAGS = -Iinclude -std=c99
SRC = src/*.c
HEADERS = include/*.h
MAIN = main.c
OUTPUT = main

TESTS = test_sleepled test_divide

all: $(OUTPUT)

$(OUTPUT): $(MAIN) $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MAIN) $(SRC) -o $(OUTPUT)

tests: $(TESTS)

$(TESTS): %: tests/%.c $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SRC) -o tests/$@

clean:
	rm -f $(OUTPUT) tests/$(TESTS)