#!/bin/bash

set -e

export MOZCONFIG=mozconfig-b2g
export PLATFORM_VERSION=23
export ANDROID_PLATFORM=android-${PLATFORM_VERSION}

# Check that the GONK_PATH environment variable is set.
if [ -z ${GONK_PATH+x} ];
then
    echo "Please set GONK_PATH to the root of your Gonk directory first.";
    exit 1;
else
    echo "Using '$GONK_PATH'";
fi

# Check that the GONK_PRODUCT_NAME environment variable is set.
if [ -z ${GONK_PRODUCT_NAME+x} ];
then
    echo "Please set GONK_PRODUCT_NAME to the name of the device (look at $GONK_PATH/out/target/product).";
    exit 1;
else
    echo "Product is '$GONK_PRODUCT_NAME'";
fi

if [ -z ${GECKO_OBJDIR+x} ]; then
    echo "Using default objdir"
else
    export MOZ_OBJDIR="$GECKO_OBJDIR"
    echo "Building in $MOZ_OBJDIR"
fi

if [ -z ${GET_FRAMEBUFFER_FORMAT_FROM_HWC+x} ]; then
    echo "GET_FRAMEBUFFER_FORMAT_FROM_HWC is not set"
else
    HWC_DEFINE="-DGET_FRAMEBUFFER_FORMAT_FROM_HWC"
    echo "Setting -DGET_FRAMEBUFFER_FORMAT_FROM_HWC"
fi

# Check if the JS shell is available. If not, download it
# to make sure we can minify JS code when packaging.
if [ -f "./jsshell/js" ]; then
	echo "JS shell found."
else
    echo "Downloading JS shell..."
    HOST_OS=$(uname -s)
    if [ "$HOST_OS" == "Darwin" ]; then
        SHELL_ARCH=mac
    else
        SHELL_ARCH=linux-x86_64
    fi

    mkdir -p jsshell
    curl https://ftp.mozilla.org/pub/firefox/releases/67.0b8/jsshell/jsshell-${SHELL_ARCH}.zip > /tmp/jsshell-${SHELL_ARCH}.zip
    cd jsshell
    unzip /tmp/jsshell-${SHELL_ARCH}.zip
    rm /tmp/jsshell-${SHELL_ARCH}.zip
    cd ..
fi
export JS_BINARY=`pwd`/jsshell/js

export MOZCONFIG=`pwd`/mozconfig-b2g

export NDK_DIR=$HOME/.mozbuild/android-ndk-r17b/

export PATH=$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64/bin:$GONK_PATH/prebuilts/linux-x86_64/bin/:$PATH

SYSROOT=$NDK_DIR/platforms/$ANDROID_PLATFORM/arch-arm/
GONK_LIBS=$GONK_PATH/out/target/product/$GONK_PRODUCT_NAME/obj/lib/

ARCH_DIR="arch-arm"

export GONK_PRODUCT=$GONK_PRODUCT_NAME

# ld --version

export CFLAGS="-DANDROID -DTARGET_OS_GONK \
-Oz \
-DJE_FORCE_SYNC_COMPARE_AND_SWAP_4=1 \
$HWC_DEFINE \
-DANDROID_VERSION=$PLATFORM_VERSION \
-D__SOFTFP__ \
-D_USING_LIBCXX \
-Wno-nullability-completeness \
-DGR_GL_USE_NEW_SHADER_SOURCE_SIGNATURE=1 \
-isystem $GONK_PATH/bionic \
-isystem $GONK_PATH/bionic/libc/$ARCH_DIR/include \
-isystem $NDK_DIR/platforms/$ANDROID_PLATFORM/arch-arm/usr/include \
-isystem $GONK_PATH/bionic/libc/include/ \
-isystem $GONK_PATH/bionic/libc/kernel/common \
-isystem $GONK_PATH/bionic/libc/kernel/$ARCH_DIR \
-isystem $GONK_PATH/bionic/libc/kernel/uapi/ \
-isystem $GONK_PATH/bionic/libc/kernel/uapi/asm-arm/ \
-isystem $GONK_PATH/bionic/libm/include \
-I$GONK_PATH/system/core/libpixelflinger/include/ \
-I$GONK_PATH/frameworks/native/include \
-I$GONK_PATH/frameworks/native/include/android \
-I$GONK_PATH/frameworks/native/libs/nativebase/include \
-I$GONK_PATH/frameworks/native/libs/nativebase \
-I$GONK_PATH/system \
-I$(pwd)/modules/freetype2/include \
-I$GONK_PATH/system/core/include \
-I$GONK_PATH/system/core/base/include \
-I$GONK_PATH/external/zlib \
-I$GONK_PATH/hardware/libhardware/include/"

export CPPFLAGS="-fPIC \
-isystem $GONK_PATH/api/cpp/include \
$CFLAGS"

export CXXFLAGS="$CPPFLAGS -std=c++14"

export RUSTC_OPT_LEVEL=z

GCC_LIB="-L$GONK_PATH/prebuilts/gcc/darwin-x86/arm/arm-linux-androideabi-4.9/lib/gcc/arm-linux-androideabi/4.9.x/"

export ANDROID_NDK=$NDK_DIR
export ANDROID_PLATFORM=$ANDROID_PLATFORM

export LDFLAGS="-L$GONK_PATH/out/target/product/$GONK_PRODUCT_NAME/obj/lib \
-Wl,-rpath-link=$GONK_PATH/out/target/product/$GONK_PRODUCT_NAME/obj/lib \
--sysroot=$SYSROOT $GCC_LIB -ldl -lstdc++ -Wl,--no-as-needed \
-llog -landroid -lbinder -lutils -lcutils -lhardware_legacy -lhardware -lui -lgui -lsuspend"

./mach build $@
