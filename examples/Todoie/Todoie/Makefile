TOPDIR=/Users/jerrymarino/Projects/objcjs/examples/Todoie
SOURCEDIR := /build/Debug-iphonesimulator

OBJCJS=/Users/jerrymarino/Projects/objcjs/build/Debug/objcjs
SOURCES = todoie.js

all: post-build

BUILD_SCRIPT = $(TOPDIR)$(SOURCEDIR)/build.sh

# objcjs compiles javascript to .s
js-build:
		rm -rf $(BUILD_SCRIPT)

		@echo export OBJCJS_ENV_BUILD_DIR="$(TOPDIR)/build/Debug-iphonesimulator/" >> $(BUILD_SCRIPT)
		@echo export OBJCJS_ENV_DEBUG_COMPILER=true >> $(BUILD_SCRIPT)

#		@echo export OBJCJS_ENV_CREATE_EXECUTABLE=false >> $(BUILD_SCRIPT)
        # compile for simulator
		@echo export OBJCJS_ENV_MTRIPEL="-mtriple=i386-unknown-ios" >> $(BUILD_SCRIPT)

#		@echo export OBJCJS_ENV_RUNTIME="" >> $(BUILD_SCRIPT)

		@echo export OBJCJS_ENV_PROJECT_ROOT_DIR="/Users/jerrymarino/Projects/objcjs/examples/Todoie/Todoie/" >> $(BUILD_SCRIPT)

		@echo $(OBJCJS) $(SOURCES) >> $(BUILD_SCRIPT)
		chmod +x $(BUILD_SCRIPT)
		$(BUILD_SCRIPT)

main-build: js-build 

post-build:	main-build
	@echo BUILD_SUCCEEDED!

clean:
	rm -rf $(OBJECTS) $(TOPDIR)$(SOURCEDIR)/objcjsapp

run: js-build
	$(TOPDIR)$(SOURCEDIR)/objcjsapp
