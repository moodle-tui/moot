MAKE = make
LDLIBS = -lcurl -lm
INCLUDE_LIB = -Ilib/
INCLUDE_MOODLE = -Imoodle/

LIB = lib
LIB_SRC = $(wildcard $(LIB)/*.c)
LIB_OBJ = $(LIB_SRC:%.c=%.o)

MOODLE = moodle
MOODLE_SRC = $(wildcard $(MOODLE)/*.c)
MOODLE_OBJ = $(MOODLE_SRC:%.c=%.o)

all: $(MOODLE_OBJ)

$(LIB)/%.o: $(LIB)/%.c
	$(CC) -c $< -o $@

$(MOODLE)/%.o: $(MOODLE)/%.c
	$(CC) -c $< $(INCLUDE_LIB) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(LIB) $(MOODLE_OBJ)

TEST = moodle/test/test
test: $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(TEST).c $^ $(INCLUDE_MOODLE) $(LDLIBS) -o $(TEST)

clean:
	rm -f $(LIB_OBJ)
	rm -f $(MOODLE_OBJ)
	rm -f $(TEST_OBJ)
	rm -f $(TEST)

.PHONY: all $(LIB) $(MOODLE) clean $(TEST)
