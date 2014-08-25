CUJS_ROOT_DIR=${PWD} 

mkdir -p deps/

echo "Installing v8.."

# Install v8 with gyp (https://code.google.com/p/v8/wiki/BuildingWithGYP)
git clone https://chromium.googlesource.com/external/v8.git deps/v8-trunk

# Explicitly use Version 3.29.16 (based on bleeding_edge revision r23326)
git checkout a8702c210b949f35c64d8e4aa01bb6d525086c85

cd deps/v8-trunk/ 
make builddeps
build/gyp_v8 -Dtarget_arch=x64

cd tools/gyp

# Build with xcodebuild
xcodebuild GCC_PREPROCESSOR_MACROS="OBJECT_PRINT V8_TARGET_ARCH_X64 ENABLE_GDB_JIT_INTERFACE V8_DEPRECATION_WARNINGS V8_I18N_SUPPORT U_USING_ICU_NAMESPACE=0 U_STATIC_IMPLEMENTATION ENABLE_DISASSEMBLER V8_ENABLE_CHECKS VERIFY_HEAP ENABLE_EXTRA_CHECKS ENABLE_HANDLE_ZAPPING __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS" -configuration "Release"

echo "Installed v8"

cd  $CUJS_ROOT_DIR

echo "Installing llvm and lib clang.."

mkdir -p deps/llvm

mkdir -p /tmp/cujs-install
cd /tmp/cujs-install

wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
wget http://llvm.org/releases/3.4/clang-3.4.src.tar.gz
wget http://llvm.org/releases/3.4/compiler-rt-3.4.src.tar.gz

# LLVM
tar xvf llvm-3.4.src.tar.gz
cd llvm-3.4/tools

# Clang Front End
tar xvf ../../clang-3.4.src.tar.gz
mv clang-3.4 clang

# Compiler RT
cd ../projects
tar xvf ../../compiler-rt-3.4.src.tar.gz
mv compiler-rt-3.4/ compiler-rt

# Build
cd ..

./configure CXX=`which clang++` CXXFLAGS="--std=c++11" --prefix=$CUJS_ROOT_DIR/deps/llvm --enable-shared

make -j4
make install

cd $CUJS_ROOT_DIR

# Compiler dependencies

mkdir -p /usr/local/bin/cujs-deps

# Use this version of clang and llc
INSTALL_PATH="/usr/local/bin/cujs-deps"
cp deps/llvm/bin/llc $INSTALL_PATH
cp deps/llvm/bin/clang $INSTALL_PATH

# clang and llc will use libLLVM in /usr/local/lib/cujs-deps so copy there
mkdir -p /usr/local/lib/cujs-deps
cp deps/llvm/lib/libLLVM-3.4.dylib /usr/local/lib/cujs-deps

echo "Installed llvm with lib clang"

