MAKE = make
LDLIBS = -lcurl -lm
INCLUDE_LIB = -Ilib/
INCLUDE_MOODLE = -Imoodle/
CCFLAGS = -Wall

LIB = lib
LIB_SRC = $(wildcard $(LIB)/*.c)
LIB_OBJ = $(LIB_SRC:%.c=%.o)

MOODLE = moodle
MOODLE_SRC = $(wildcard $(MOODLE)/*.c)
MOODLE_OBJ = $(MOODLE_SRC:%.c=%.o)

UI = ui
UI_SRC = $(wildcard $(UI)/*.c)
UI_OBJ = $(UI_SRC:%.c=%.o)

all: $(MOODLE_OBJ)

$(LIB)/%.o: $(LIB)/%.c
	$(CC) -c $< -o $@

$(MOODLE)/%.o: $(MOODLE)/%.c
	$(CC) -c $< $(INCLUDE_LIB) -o $@

$(UI_OBJ): $(UI_SRC) $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(CCFLAGS) $^ $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(LIB) $(MOODLE_OBJ)

$(UI): $(UI_OBJ)

clean:
	rm -f $(LIB_OBJ)
	rm -f $(MOODLE_OBJ)
	rm -f $(UI_OBJ)

.PHONY: all $(LIB) $(MOODLE) clean test
