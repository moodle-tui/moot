include config.mk

LDLIBS = -lcurl -lm -ldl
INCLUDE_LIB = -Ilib/
INCLUDE_MOODLE = -Imoodle/

LIB = lib
LIB_SRC = $(wildcard $(LIB)/*.c)
LIB_OBJ = $(LIB_SRC:%.c=%.o)

MOODLE = moodle
MOODLE_REQ = $(LIB)/json.o $(LIB)/dlib.o
MOODLE_SRC = $(wildcard $(MOODLE)/*.c)
MOODLE_OBJ = $(MOODLE_SRC:%.c=%.o)

UI = ui
UI_SRC = $(wildcard $(UI)/*.c)
UI_OBJ = $(UI_SRC:%.c=%.o)

PLUGINS = $(MOODLE)/plugins
PLUGIN_EXT = mtplug

all: moot vu_sso_plugin

$(LIB)/%.o: $(LIB)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(MOODLE)/%.o: $(MOODLE)/%.c
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_LIB) $(INCLUDES) -o $@

$(UI)/%.o: $(UI)/%.c $(MOODLE_OBJ) $(LIB_OBJ) $(LIB)/rlutil.h
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(MOODLE_REQ) $(MOODLE_OBJ)

$(UI): $(UI_OBJ)

moot: $(UI_OBJ) $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(CCFLAGS) $^ $(LDLIBS) $(LIBS) -o $(TARGET)

TEST = moodle/test/test
test: $(MOODLE_OBJ) $(LIB_OBJ) vu_sso_plugin
	$(CC) $(CCFLAGS) $(TEST).c $(MOODLE_OBJ) $(LIB_OBJ) $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) $(LIBS) -o $(TEST)

VU_SSO = $(PLUGINS)/vu_sso
vu_sso_plugin: $(LIB)/base64.o
	$(CC) $(CCFLAGS) -shared $(VU_SSO).c $^ $(INCLUDE_LIB) $(LDLIBS) $(LIBS) -o $(VU_SSO).$(PLUGIN_EXT)

clean:
	$(RM) $(LIB_OBJ)
	$(RM) $(MOODLE_OBJ)
	$(RM) $(UI_OBJ)
	$(RM) $(TARGET)
	$(RM) $(TEST)
	$(RM) $(VU_SSO).$(PLUGIN_EXT)

.PHONY: all $(LIB) $(MOODLE) moot clean test vu_sso_plugin
