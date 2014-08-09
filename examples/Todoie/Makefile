ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build/Debug-iphonesimulator

CUJS=/usr/local/bin/cujs
SOURCES = todoie.js

all: post-build

BUILD_SCRIPT = $(BUILD_DIR)/cujs-build.sh

# cujs compiles javascript to .s
js-build:
		if [ -a $(BUILD_SCRIPT) ] ;\
			then \
			rm -rf $(BUILD_SCRIPT); \
		fi

		mkdir -p $(BUILD_DIR)\ ; 

		@echo export CUJS_ENV_BUILD_DIR="$(ROOT_DIR)/build/Debug-iphonesimulator/" >> $(BUILD_SCRIPT)

# Don't dump bitcode
#		@echo export CUJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

        # compile for simulator
		@echo export CUJS_ENV_MTRIPEL="-mtriple=i386-unknown-ios" >> $(BUILD_SCRIPT)

		@echo export CUJS_ENV_PROJECT_ROOT_DIR=$(ROOT_DIR) >> $(BUILD_SCRIPT)

		@echo $(CUJS) $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!
