include config.mk

LDLIBS = -lcurl -lm
INCLUDE_LIB = -Ilib/
INCLUDE_MOODLE = -Imoodle/

LIB = lib
LIB_SRC = $(wildcard $(LIB)/*.c)
LIB_OBJ = $(LIB_SRC:%.c=%.o)

MOODLE = moodle
MOODLE_SRC = $(wildcard $(MOODLE)/*.c)
MOODLE_OBJ = $(MOODLE_SRC:%.c=%.o)

UI = ui
UI_SRC = $(wildcard $(UI)/*.c)
UI_OBJ = $(UI_SRC:%.c=%.o)

all: moot

$(LIB)/%.o: $(LIB)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(MOODLE)/%.o: $(MOODLE)/%.c
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_LIB) $(INCLUDES) -o $@

$(UI_OBJ): $(UI_SRC) $(MOODLE_OBJ) $(LIB_OBJ) $(LIB)/rlutil.h
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(LIB) $(MOODLE_OBJ)

$(UI): $(UI_OBJ)

moot: $(UI_OBJ) $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(CCFLAGS) $^ $(LDLIBS) $(LIBS) -o $(TARGET)

TEST = moodle/test/test
test: $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(CCFLAGS) $(TEST).c $^ $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) $(LIBS) -o $(TEST)

clean:
	$(RM) $(LIB_OBJ)
	$(RM) $(MOODLE_OBJ)
	$(RM) $(UI_OBJ)
	$(RM) $(TARGET)
	$(RM) $(TEST)

.PHONY: all $(LIB) $(MOODLE) moot clean test
