CC=clang # or gcc

FRAMEWORKS:= -framework Foundation
LIBRARIES:= -lobjc

module.o:/tmp/module.s
    $(AS)
CFLAGS=-Wall -Werror -g -v $(SOURCE)
LDFLAGS=$(LIBRARIES) $(FRAMEWORKS)
OUT=-o module

all:
	    $(CC) $(CFLAGS) $(LDFLAGS) $(OUT)
