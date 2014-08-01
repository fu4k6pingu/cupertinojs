# installing

- install v8 with gyp
https://code.google.com/p/v8/wiki/BuildingWithGYP

make builddeps (last used revision 258359)

build/gyp_v8 -Dtarget_arch=x64


FIXME: currently links stl and new apples new c++ lib

- link llvm 

### basic install with homebrew

brew install llvm --with-clang++

link with libLLVM-3.4.dlyb (usr/local/Cellar/llvm/)

- for systems with Mavericks:
    link with libstdc++.X.X.X.dylb (usr/lib/)
    
