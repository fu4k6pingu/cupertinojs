# installing

- install v8 with gyp
https://code.google.com/p/v8/wiki/BuildingWithGYP

make builddeps (last used revision)

build/gyp_v8 -Dtarget_arch=x64


FIXME: currently links stl and new apples new c++ lib

- link llvm 

### basic install with homebrew

brew install llvm --with-clang++ --with-clang

make sure runtime search paths include /usr/local/opt/llvm/lib/

link with libLLVM-3.4.dlyb (usr/local/Cellar/llvm/)

- for systems with Mavericks:
    link with libstdc++.X.X.X.dylb (usr/lib/)
    
