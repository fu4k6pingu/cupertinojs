ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build/Debug-iphonesimulator

OBJCJS=/usr/local/bin/objcjs
SOURCES = todoie.js

all: post-build

BUILD_SCRIPT = $(BUILD_DIR)/objcjs-build.sh

# objcjs compiles javascript to .s
js-build:
		if [ -a $(BUILD_SCRIPT) ] ;\
			then \
			rm -rf $(BUILD_SCRIPT); \
		fi

		mkdir -p $(BUILD_DIR)\ ; 

		@echo export OBJCJS_ENV_BUILD_DIR="$(ROOT_DIR)/build/Debug-iphonesimulator/" >> $(BUILD_SCRIPT)

# Don't dump bitcode
#		@echo export OBJCJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

        # compile for simulator
		@echo export OBJCJS_ENV_MTRIPEL="-mtriple=i386-unknown-ios" >> $(BUILD_SCRIPT)

		@echo export OBJCJS_ENV_PROJECT_ROOT_DIR=$(ROOT_DIR) >> $(BUILD_SCRIPT)

		@echo $(OBJCJS) $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!
