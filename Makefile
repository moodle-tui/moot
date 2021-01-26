include config.mk

INCLUDE_LIB = -Ilib/
INCLUDE_MOODLE = -Imoodle/

LIB = lib
LIB_SRC = $(wildcard $(LIB)/*.c)
LIB_OBJ = $(LIB_SRC:%.c=%.o)

GUMBO = $(LIB)/gumbo
GUMBO_SRC = $(wildcard $(GUMBO)/*.c)
GUMBO_OBJ = $(GUMBO_SRC:%.c=%.o)

MOODLE = moodle
MOODLE_REQ = $(LIB)/json.o $(LIB)/dlib.o
MOODLE_SRC = $(wildcard $(MOODLE)/*.c)
MOODLE_OBJ = $(MOODLE_SRC:%.c=%.o)

APP = app
APP_SRC = $(wildcard $(APP)/*.c)
APP_OBJ = $(APP_SRC:%.c=%.o)

PLUGINS = $(MOODLE)/plugins
PLUGIN_EXT = mtplug

all: moot vu_sso_plugin

$(LIB)/%.o: $(LIB)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(GUMBO)/%.o: $(GUMBO)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(MOODLE)/%.o: $(MOODLE)/%.c
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_LIB) $(INCLUDES) -o $@

$(APP)/%.o: $(APP)/%.c
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_MOODLE) $(INCLUDE_LIB) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(MOODLE_REQ) $(MOODLE_OBJ)

$(APP): $(LIB) $(APP_OBJ) $(GUMBO_OBJ)

moot: $(APP_OBJ) $(MOODLE_OBJ) $(LIB_OBJ) $(GUMBO_OBJ)
	$(CC) $(CCFLAGS) $^ $(LDLIBS) $(LIBS) -o $(TARGET)

TEST = moodle/test/test
test: $(MOODLE_OBJ) $(LIB_OBJ) vu_sso_plugin
	$(CC) $(CCFLAGS) $(TEST).c $(MOODLE_OBJ) $(LIB_OBJ) $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) $(LIBS) $(INCLUDES) -o $(TEST)$(EXEC_EXT)

JSON_TEST = lib/tests/json
json_test: $(LIB)/json.o $(LIB)/utf8.o
	$(CC) $(CCFLAGS) $(JSON_TEST).c $^ $(INCLUDE_LIB) $(LIBS) $(INCLUDES) -o $(JSON_TEST)$(EXEC_EXT)

VU_SSO = $(PLUGINS)/vu_sso
vu_sso_plugin: $(LIB)/base64.o
	$(CC) $(CCFLAGS) -shared $(VU_SSO).c $^ $(INCLUDE_LIB) $(INCLUDE_MOODLE) $(LDLIBS) $(LIBS) $(INCLUDES) -o $(VU_SSO).$(PLUGIN_EXT)

clean:
	$(RM) $(subst /,$(SEP),$(LIB_OBJ))
	$(RM) $(subst /,$(SEP),$(GUMBO_OBJ))
	$(RM) $(subst /,$(SEP),$(MOODLE_OBJ))
	$(RM) $(subst /,$(SEP),$(APP_OBJ))
	$(RM) $(subst /,$(SEP),$(TARGET))
	$(RM) $(subst /,$(SEP),$(TEST)$(EXEC_EXT))
	$(RM) $(subst /,$(SEP),$(VU_SSO).$(PLUGIN_EXT))
	$(RM) $(subst /,$(SEP),$(JSON_TEST)$(EXEC_EXT))

.PHONY: all $(LIB) $(MOODLE) $(APP) moot clean test vu_sso_plugin
