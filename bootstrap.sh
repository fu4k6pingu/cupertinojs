mkdir -p deps/

# Install v8 with gyp (https://code.google.com/p/v8/wiki/BuildingWithGYP)
git clone https://chromium.googlesource.com/external/v8.git deps/v8-trunk

## Explicitly use Version 3.29.16 (based on bleeding_edge revision r23326)
git checkout f284b29e37d97d7ee9128055862179dcbda7e398

cd deps/v8-trunk/ 

make builddeps

build/gyp_v8 -Dtarget_arch=x64

cd ../..

# install llvm
brew install llvm --with-clang++ --with-clang

