ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDDIR := /tmp/objcjs-test

SOURCES = test.js

all: post-build

BUILD_SCRIPT = $(BUILDDIR)/build.sh

# objcjs compiles javascript to .s
js-build:
		if [ -a $(BUILD_SCRIPT) ] ;\
			then \
			rm -rf $(BUILD_SCRIPT); \
		fi
		mkdir -p $(BUILDDIR) ; \

		@echo export OBJCJS_ENV_BUILD_DIR=$(BUILDDIR) >> $(BUILD_SCRIPT)

# Don't enable compiler debug (bitcode dump)
#		@echo export OBJCJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

		@echo export OBJCJS_ENV_RUNTIME="/usr/local/lib/objcjs-runtime.dylib" >> $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_CREATE_EXECUTABLE=true >> $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_MTRIPEL="" >> $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_PROJECT_ROOT_DIR=$(ROOT_DIR) >> $(BUILD_SCRIPT)
		@echo /usr/local/bin/objcjs $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!

clean:
	rm -rf $(BUILDDIR)

run: js-build
	$(BUILDDIR)/objcjsapp
