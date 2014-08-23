# Cupertino.js - Compile Javascript to Cocoa

Cupertino.js is a toolchain for building Cocoa applications with Javascript. 

It includes a static Javascript compiler and runtime.

## State of union:

Although the example app runs and builds in the simulator, Cupertino.js is not
yet production ready.

There several features not yet implemented including:

- Standard JS lib (Strings, arrays, etc)
- Garbage collection - plan is to use a variant of ARC
- JS modules
- Switch statements

## Project setup

Cupertino.js has the following dependencies:

- v8 
- llvm

There is a script which should set everything up.

### Script install

    ./bootstrap.sh

### Manual install

- install v8 to deps/ with gyp

    https://code.google.com/p/v8/wiki/BuildingWithGYP

    make builddeps

    build/gyp_v8 -Dtarget_arch=x64

- install llvm with clang support

    brew install llvm --with-clang++ --with-clang

