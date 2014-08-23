mkdir -p deps/

# Install v8 with gyp (https://code.google.com/p/v8/wiki/BuildingWithGYP)
git clone https://chromium.googlesource.com/external/v8.git deps/v8-trunk

cd deps/v8-trunk/ 

make builddeps

build/gyp_v8 -Dtarget_arch=x64

## If no gyp do this:
# svn co http://gyp.googlecode.com/svn/trunk build/gyp --revision 1831

cd ../..

# install llvm
brew install llvm --with-clang++ --with-clang

