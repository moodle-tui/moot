CC = gcc
INPUT = ui.c
OBJECT = ui.o
DEPENDENCIES= ui.c ../moodle/client.h ../moodle/util.h
INCLUDE_PATH = -I../lib/ -I../moodle/

all: $(OBJECT)

$(OBJECT): $(DEPENDENCIES)
	$(CC) $(INPUT) -o $@ $(INCLUDE_PATH)

.PHONY: clean
clean:
	rm -f $(OBJECT)
