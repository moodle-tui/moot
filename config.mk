CC = gcc
RM = rm -f
LDLIBS = -lcurl -lm -ldl
CCFLAGS = -Wall -g -Wpedantic -std=c11
# e. g. -I.../curl/include
INCLUDES =
# e. g. -L.../curl/lib
LIBS = 
# e. g. .exe for win
EXEC_EXT=
#separator only replaced when deleting files, \ on windows.
SEP = $(strip /)
TARGET = moot$(EXEC_EXT)
