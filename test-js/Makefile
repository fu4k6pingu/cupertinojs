TOPDIR=/Users/jerrymarino/Projects/objcjs

CC=clang
FRAMEWORKS:= -framework Foundation \
		/usr/local/lib/objcjs-runtime.dylib
LIBS:= -lobjc 

OBJDIR := /build/Debug
SOURCES = test.js

# OBJECTS = $(TOPDIR)$(OBJDIR)/$(SOURCES:.js=.s)

CFLAGS=-Wall -Werror $(SOURCE)
LLVMLDFLAGS=$(LIBS) $(FRAMEWORKS)

all: post-build


# objcjs compiles javascripts to .s
js-build:
		$(TOPDIR)$(OBJDIR)/objcjs $(SOURCES)

# compile and link .s with CC
# EX:
# clang  -o /tmp/a.out
# -framework Foundation
# /Users/jerrymarino/Projects/objcjs/build/Debug/objcjs-runtime.dylib
# /tmp/module.s 
# target: $(OBJECTS)
#	#$(CC) -o $@ $< $(LLVMLDFLAGS)
#	@echo $(OBJECTS)

# run build
main-build: js-build 
# objcjs outputs a binary by default
#	    @$(MAKE) target 

post-build:	main-build
	@echo BUILD_SUCCEEDED!

clean:
	rm -rf $(OBJECTS) target


run: post-build
	$(TOPDIR)$(OBJDIR)/objcjsapp
