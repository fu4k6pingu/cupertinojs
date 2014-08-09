ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := /tmp/cujs-test

SOURCES = test.js

all: post-build

BUILD_SCRIPT = $(BUILD_DIR)/build.sh

# cujs compiles javascript to .s
js-build:
		if [ -a $(BUILD_SCRIPT) ] ;\
			then \
			rm -rf $(BUILD_SCRIPT); \
		fi

		mkdir -p $(BUILD_DIR) ; 

		@echo export CUJS_ENV_BUILD_DIR=$(BUILD_DIR) >> $(BUILD_SCRIPT)

# Don't enable compiler debug (bitcode dump)
#		@echo export CUJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

		@echo export CUJS_ENV_RUNTIME="/usr/local/lib/cujs-runtime.dylib" >> $(BUILD_SCRIPT)
		@echo export CUJS_ENV_CREATE_EXECUTABLE=true >> $(BUILD_SCRIPT)
		@echo export CUJS_ENV_MTRIPEL="" >> $(BUILD_SCRIPT)
		@echo export CUJS_ENV_PROJECT_ROOT_DIR=$(ROOT_DIR) >> $(BUILD_SCRIPT)
		@echo /usr/local/bin/cujs $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!

clean:
	rm -rf $(BUILD_DIR)

run: js-build
	$(BUILD_DIR)/cujsapp
