include config.mk

LDLIBS = -lcurl -lm -ldl
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

$(APP)/%.o: $(APP)/%.c $(LIB_OBJ) $(MOODLE_OBJ)
	$(CC) $(CCFLAGS) -c $< $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) -o $@

$(LIB): $(LIB_OBJ)

$(MOODLE): $(MOODLE_REQ) $(MOODLE_OBJ)

$(APP): $(LIB) $(APP_OBJ)

moot: $(APP_OBJ) $(MOODLE_OBJ) $(LIB_OBJ)
	$(CC) $(CCFLAGS) $^ $(LDLIBS) $(LIBS) -o $(TARGET)


HTML = app/html_renderer
html_renderer: $(GUMBO_OBJ) $(LIB)/utf8.o $(LIB)/wcwidth.o
	$(CC) $(CCFLAGS) $(HTML).c $^ $(INCLUDE_LIB) $(LDLIBS) -o $(HTML)

TEST = moodle/test/test
test: $(MOODLE_OBJ) $(LIB_OBJ) vu_sso_plugin
	$(CC) $(CCFLAGS) $(TEST).c $(MOODLE_OBJ) $(LIB_OBJ) $(INCLUDE_MOODLE) $(INCLUDE_LIB) $(LDLIBS) $(LIBS) -o $(TEST)

JSON_TEST = lib/tests/json
json_test: $(LIB)/json.o $(LIB)/utf8.o
	$(CC) $(CCFLAGS) $(JSON_TEST).c $^ $(INCLUDE_LIB) $(LDLIBS) $(LIBS) -o $(JSON_TEST)

VU_SSO = $(PLUGINS)/vu_sso
vu_sso_plugin: $(LIB)/base64.o
	$(CC) $(CCFLAGS) -shared $(VU_SSO).c $^ $(INCLUDE_LIB) $(INCLUDE_MOODLE) $(LDLIBS) $(LIBS) -o $(VU_SSO).$(PLUGIN_EXT)

clean:
	$(RM) $(LIB_OBJ)
	$(RM) $(GUMBO_OBJ)
	$(RM) $(MOODLE_OBJ)
	$(RM) $(APP_OBJ)
	$(RM) $(TARGET)
	$(RM) $(TEST)
	$(RM) $(VU_SSO).$(PLUGIN_EXT)
	$(RM) $(JSON_TEST)

.PHONY: all $(LIB) $(MOODLE) moot clean test vu_sso_plugin
