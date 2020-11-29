CC = gcc
ROOT = ..
LIBS = -lcurl -lm
INCLUDE = -I$(ROOT)/lib/
LIBDIR = $(ROOT)/lib
OBJFILES = util.o client.o test.o
TARGET = test
MAKE = make

all: $(TARGET)

lib:
	$(MAKE) -C $(LIBDIR)

%.o: %.c lib
	${CC} -c $(INCLUDE) $< $(LIBS)

$(TARGET): $(OBJFILES)
	$(CC) $(INCLUDE) -o $(TARGET) $(LIBDIR)/*.o $(OBJFILES) $(LIBS)

clean:
	$(MAKE) -C $(LIBDIR) clean
	rm *.o
	rm $(TARGET)

.PHONY: all lib clean