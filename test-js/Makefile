TOPDIR=/Users/jerrymarino/Projects/objcjs
SOURCEDIR := /build/Debug

SOURCES = test.js

all: post-build

BUILD_SCRIPT = $(TOPDIR)$(SOURCEDIR)/build.sh

# objcjs compiles javascript to .s
js-build:
		rm -rf $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_BUILD_DIR="$(TOPDIR)/build/Debug/" >> $(BUILD_SCRIPT)
#		@echo export OBJCJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_RUNTIME="/Users/jerrymarino/Projects/objcjs/build/Debug/objcjs-runtime.dylib" >> $(BUILD_SCRIPT)
		@echo $(TOPDIR)$(SOURCEDIR)/objcjs $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!

clean:
	rm -rf $(OBJECTS) $(TOPDIR)$(SOURCEDIR)/objcjsapp

run: js-build
	$(TOPDIR)$(SOURCEDIR)/objcjsapp
