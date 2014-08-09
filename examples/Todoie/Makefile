ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build/Debug-iphonesimulator
SOURCE_DIR := $(ROOT_DIR)/Todoie

CUJS=/usr/local/bin/cujs
SOURCES = todoie.js

all: post-build

BUILD_SCRIPT = $(BUILD_DIR)/cujs-build.sh

# cujs compiles javascript to .s
js-build:
        # make a new build script for every run
		if [ -a $(BUILD_SCRIPT) ] ;\
			then \
			rm -rf $(BUILD_SCRIPT); \
		fi

		mkdir -p $(ROOT_DIR)/build ;
		mkdir -p $(BUILD_DIR) ; 

        # CUJS_ENV_BUILD_DIR is where the build products are installed
		@echo export CUJS_ENV_BUILD_DIR="$(ROOT_DIR)/build/Debug-iphonesimulator" >> $(BUILD_SCRIPT)

        # CUJS_ENV_DEBUG_COMPILER=true will dump bitcode to stdio
#		@echo export CUJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

        # compile for simulator
		@echo export CUJS_ENV_MTRIPEL="-mtriple=i386-unknown-ios" >> $(BUILD_SCRIPT)

        # CUJS_ENV_PROJECT_ROOT_DIR is the base path for imports
		@echo export CUJS_ENV_PROJECT_ROOT_DIR=$(SOURCE_DIR) >> $(BUILD_SCRIPT)

		@echo $(CUJS) $(SOURCE_DIR)/$(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)

		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!
