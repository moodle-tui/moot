CC = gcc
LIBS = -lm
OBJFILES = json.o

all: $(OBJFILES)

%.o: %.c
	${CC} -c $(INCLUDE) $< $(LIBS)
clean:
	rm *.o

.PHONY: all clean
