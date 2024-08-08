[![Github actions](https://github.com/infineon/optiga-tpm-aosp/actions/workflows/main.yml/badge.svg)](https://github.com/infineon/optiga-tpm-aosp/actions)

# Introduction

Guide to Integrating OPTIGA™ TPM 2.0 with the Android Open Source Project (AOSP).

This project will guide you through the necessary steps to cross-compile essential TPM2-related libraries and incorporate them into the AOSP build. Additionally, the guide includes instructions for launching the AOSP emulator with the TPM2 simulator, enabling testing of the example Android application.

Please note that this guide is focused exclusively on TPM integration and does not cover AOSP security configurations. Be aware that the emulator is set to operate in permissive mode ("-selinux permissive") as described in this guide. Ensure you implement the necessary security measures according to your application's needs.

---

# Table of Contents

- **[Prerequisites](#prerequisites)**
- **[Preparing the Environment](#preparing-the-environment)**
- **[Default AOSP Build](#default-aosp-build)**
    - **[Building AOSP](#building-aosp)**
    - **[Running Android Emulator with GUI](#running-android-emulator-with-gui)**
    - **[Running Android Emulator Headless](#running-android-emulator-headless)**
- **[Cross-Compiling Packages](#cross-compiling-packages)**
    - **[Cross-Compiling OpenSSL 3](#cross-compiling-openssl-3)**
    - **[Cross-Compiling TPM2-TSS](#cross-compiling-tpm2-tss)**
    - **[Cross-Compiling TPM2-OpenSSL](#cross-compiling-tpm2-openssl)**
    - **[Cross-Compiling TPM2 Simulator](#cross-compiling-tpm2-simulator)**
- **[Example Android Application for Accessing TPM](#example-android-application-for-accessing-tpm)**
    - **[Preparing the Example Application](#preparing-the-example-application)**
    - **[Rebuilding AOSP with the Example Application](#rebuilding-aosp-with-the-example-application)**
    - **[Running the Example Application](#running-the-example-application)**
- **[License](#license)**

---

# Prerequisites

The integration guide has been CI tested for compatibility with the following platform and operating system:

- Platform: x86_64
- Operating System: Ubuntu (22.04)

---

# Preparing the Environment

Download package information:
```all
$ sudo apt update
```

Install packages necessary for AOSP building:
```all
$ sudo apt install -y git git-core gnupg flex bison build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 libncurses5 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig

$ sudo apt install -y python3
$ sudo ln -s /usr/bin/python3 /usr/bin/python
$ python --version

$ export REPO=$(mktemp /tmp/repo.XXXXX)
$ curl -o ${REPO} https://storage.googleapis.com/git-repo-downloads/repo
$ sudo install -m 755 ${REPO} /usr/bin/repo
$ repo version
```

Install packages necessary for tpm2-tss building:
```all
$ sudo apt -y install autoconf-archive libcmocka0 libcmocka-dev procps iproute2 build-essential git pkg-config gcc libtool automake libssl-dev uthash-dev autoconf doxygen libjson-c-dev libini-config-dev libcurl4-openssl-dev uuid-dev pandoc acl libglib2.0-dev xxd jq patchelf
```

Download the Android NDK:
```all
$ sudo apt install -y wget

$ cd ~
$ wget https://dl.google.com/android/repository/android-ndk-r26b-linux.zip
$ unzip android-ndk-r26b-linux.zip
$ du -ms android-ndk-r26b
```

Download this project for later use:
```exclude
$ git clone https://github.com/infineon/optiga-tpm-aosp --depth=1 ~/optiga-tpm-aosp
```

---

# Default AOSP Build

As the title suggests, in this section, we will walk through the default AOSP build to ensure our environment is correctly set up.

## Building AOSP

```all
$ mkdir ~/aosp
$ cd ~/aosp
$ repo init --depth=1 -u https://android.googlesource.com/platform/manifest -b android-14.0.0_r11
$ cp ~/aosp/.repo/repo/repo /usr/bin/repo
$ repo sync -c -f --force-sync --no-clone-bundle --no-tags -j 0
$ du -sm ~/aosp

$ cd ~/aosp
$ source build/envsetup.sh
$ lunch sdk_phone_x86_64
$ m -j
$ du -sm ~/aosp/out
```

## Running Android Emulator with GUI

```exclude
$ cd ~/aosp
$ source build/envsetup.sh
$ lunch sdk_phone_x86_64
$ emulator -verbose -gpu swiftshader -selinux permissive -logcat *:v
```

## Running Android Emulator Headless

Start the emulator in headless mode:
```all
$ cd ~/aosp
$ source build/envsetup.sh
$ lunch sdk_phone_x86_64
$ export ANDROID_EMULATOR_WAIT_TIME_BEFORE_KILL=1

$ emulator -selinux permissive -no-window -no-audio &
$ EMULATOR_PID=$!
```

Wait until the emulator is ready for use:
```all
$ export PATH=${HOME}/aosp/out/host/linux-x86/bin:$PATH

# Usage: ./emulator_check.sh <emulator pid> <desired android version> <max wait time in seconds>
$ chmod +x ~/optiga-tpm-aosp/scripts/emulator_check.sh
$ ~/optiga-tpm-aosp/scripts/emulator_check.sh $EMULATOR_PID 14 120
```

Interact with the emulator:
```all
$ adb shell getprop ro.build.version.sdk
$ adb shell getprop ro.system.build.version.release
```

Terminate the emulator:
```all
$ kill -SIGTERM $EMULATOR_PID
```

---

# Cross-Compiling Packages

The example provided here is based on the x86_64 architecture for running on the Android emulator. Modifying it for cross-compilation on other architectures should not be difficult.

## Cross-Compiling OpenSSL 3

Set the environment variables:
```all
$ export PATH=${HOME}/android-ndk-r26b/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
$ export ANDROID_NDK_ROOT=${HOME}/android-ndk-r26b
```

Download the project:
```all
$ git clone https://github.com/openssl/openssl -b openssl-3.1.4 --depth=1 ~/openssl
```

Cross-compile the project:
```all
$ cd ~/openssl

# Why "-U__ANDROID_API__ -D__ANDROID_API__=34"?
# Read https://github.com/openssl/openssl/issues/18561
$ ./Configure android-x86_64 -U__ANDROID_API__ -D__ANDROID_API__=34 \
  --libdir="${HOME}/openssl/build/usr/local/lib" \
  --prefix="${HOME}/openssl/build/usr/local" \
  --openssldir="${HOME}/openssl/build/usr/local/lib/ssl"
$ make -j

$ mkdir build
$ make install_sw -j
$ ls -R build
```

You will need to change the library name due to a conflict with AOSP BoringSSL, which uses the same library name:
```all
$ cd ~/openssl/build/usr/local/lib

# Read the SONAME entry before modifying
$ readelf -a libcrypto.so | grep SONAME

# Modify SONAME
$ patchelf --set-soname libossl3-crypto.so libcrypto.so
$ mv libcrypto.so libossl3-crypto.so

# Read the SONAME entries after making modifications
$ readelf -a libossl3-crypto.so | grep SONAME
```

## Cross-Compiling TPM2-TSS

Set the environment variables:
```all
$ export PATH=${HOME}/android-ndk-r26b/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
```

Download the project:
```all
$ git clone https://github.com/tpm2-software/tpm2-tss ~/tpm2-tss
$ cd ~/tpm2-tss
$ git checkout d0632dabe8557754705f8d38ffffdafc9f4865d1
```

Cross-compile the project:
```all
$ cd ~/tpm2-tss

$ ./bootstrap
$ ./configure --host=x86_64-linux-android34 --with-crypto=ossl \
  --disable-fapi --disable-policy --disable-tcti-spi-helper --disable-tcti-i2c-helper \
  --with-maxloglevel=none \
  --libdir="${HOME}/tpm2-tss/build/usr/local/lib" \
  --includedir="${HOME}/tpm2-tss/build/usr/local/include" \
  CC=x86_64-linux-android34-clang \
  PKG_CONFIG_PATH="${HOME}/openssl/build/usr/local/lib/pkgconfig"
$ make -j

$ make install -j
$ ls -R build
```

## Cross-Compiling TPM2-OpenSSL

Set the environment variables:
```all
$ export PATH=${HOME}/android-ndk-r26b/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
```

Download the project:
```all
$ git clone https://github.com/tpm2-software/tpm2-openssl -b 1.2.0 --depth=1 ~/tpm2-openssl
```

Cross-compile the project:
```all
$ cd ~/tpm2-openssl

$ ./bootstrap
$ ./configure --host=x86_64-linux-android34 \
  CC=x86_64-linux-android34-clang \
  PKG_CONFIG_PATH="${HOME}/openssl/build/usr/local/lib/pkgconfig:${HOME}/tpm2-tss/build/usr/local/lib/pkgconfig"
$ make -j

$ make install -j
$ ls -R ~/openssl/build
```

Change the library name to ensure clarity:
```all
$ cd ~/openssl/build/usr/local/lib/ossl-modules

# Read the SONAME entry before modifying
$ readelf -a tpm2.so | grep SONAME

# Modify SONAME
$ patchelf --set-soname libtss2-ossl3-provider.so tpm2.so
$ mv tpm2.so libtss2-ossl3-provider.so

# Read the SONAME entries after making modifications
$ readelf -a libtss2-ossl3-provider.so | grep SONAME
```

## Cross-Compiling TPM2 Simulator

Set the environment variables:
```all
$ export PATH=${HOME}/android-ndk-r26b/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
```

Download the project:
>  Support for OpenSSL 3 is not officially provided yet in the [ms-tpm-20-ref](https://github.com/microsoft/ms-tpm-20-ref) project. Official support for OpenSSL 3 can be tracked through the project's [pull request #93](https://github.com/microsoft/ms-tpm-20-ref/pull/93). This is a temporary workaround taken from the pull request.
```all
$ git clone https://github.com/microsoft/ms-tpm-20-ref ~/ms-tpm-20-ref
$ cd ~/ms-tpm-20-ref
$ git checkout e9fc7b89d865536c46deb63f9c7d0121a3ded49c
$ git apply ~/optiga-tpm-aosp/patches/ms-tpm-20-ref.patch
```

Disabling the FILE_BACKED_NV feature is not possible through the use of CFLAGS; therefore, the code must be modified:
```all
$ cd ~/ms-tpm-20-ref/TPMCmd
$ sed -i 's/define FILE_BACKED_NV YES/define FILE_BACKED_NV NO/g' Platform/include/PlatformData.h
```

Cross-compile the project:
```all
$ cd ~/ms-tpm-20-ref/TPMCmd

$ ./bootstrap
$ ./configure --host=x86_64-linux-android34 \
  CC=x86_64-linux-android34-clang \
  CPP="x86_64-linux-android34-clang -E" \
  PKG_CONFIG_PATH="${HOME}/openssl/build/usr/local/lib/pkgconfig"
$ make -j

$ mkdir build
$ make install DESTDIR=${HOME}/ms-tpm-20-ref/TPMCmd/build -j
$ ls -R build
```

---

# Example Android Application for Accessing TPM

In this section, we will launch the Android application in the Android emulator to access a simulated TPM.

## Preparing the Example Application

Complete the application template by filling in the missing components, including header files, libraries, and binaries.
> To access a real TPM target, the library `libtss2-tcti-device.so` is required too. Additionally, locate the variable `tcti_name_conf` in `~/optiga-tpm-aosp/src/IFXApp/jni/lib.c` and modify it to use a different TPM target.
```all
# Create a copy of the application
$ cp -rf ~/optiga-tpm-aosp/src/IFXApp ~/

# Openssl 3 header files and libraries
$ cp -rf ~/openssl/build/usr/local/include/openssl ~/IFXApp/ossl3/include/
$ cp ~/openssl/build/usr/local/lib/libossl3-crypto.so ~/IFXApp/ossl3/x86_64/

# tpm2-tss header files and libraries
$ cp -rf ~/tpm2-tss/build/usr/local/include/tss2 ~/IFXApp/tss2/include/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-esys.so ~/IFXApp/tss2/x86_64/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-sys.so ~/IFXApp/tss2/x86_64/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-mu.so ~/IFXApp/tss2/x86_64/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-rc.so ~/IFXApp/tss2/x86_64/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-tctildr.so ~/IFXApp/tss2/x86_64/
$ cp ~/tpm2-tss/build/usr/local/lib/libtss2-tcti-mssim.so ~/IFXApp/tss2/x86_64/
# $ cp ~/tpm2-tss/build/usr/local/lib/libtss2-tcti-device.so ~/IFXApp/tss2/<arch>/

# tpm2-openssl provider
$ cp ~/openssl/build/usr/local/lib/ossl-modules/libtss2-ossl3-provider.so ~/IFXApp/tss2-ossl3/x86_64/

# ms-tpm-20-ref TPM simulator binary
$ cp ~/ms-tpm-20-ref/TPMCmd/build/usr/local/bin/tpm2-simulator ~/IFXApp/mssim/x86_64/bin/
```

## Rebuilding AOSP with the Example Application

Remember to complete the section [Preparing the Example Application](#preparing-the-example-application) before copying the application into the AOSP project:
```all
$ cp -rf ~/IFXApp ~/aosp/packages/apps/
```

Incorporate the application into the AOSP build:
```all
$ cat << EOF >> "$HOME/aosp/build/make/target/product/base_product.mk"
$
$ PRODUCT_PACKAGES += \\
$     ifx_demo_app \\
$     tpm2-simulator \\
$     libtss2-ossl3-provider \\
$
$ PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += \\
$     system/app/ifx_demo_app/ifx_demo_app.apk \\
$     system/app/ifx_demo_app/lib/x86_64/libifx_jni.so \\
$     system/bin/tpm2-simulator \\
$     system/lib/libossl3-crypto.so \\
$     system/lib/libtss2-esys.so \\
$     system/lib/libtss2-mu.so \\
$     system/lib/libtss2-ossl3-provider.so \\
$     system/lib/libtss2-rc.so \\
$     system/lib/libtss2-sys.so \\
$     system/lib/libtss2-tctildr.so \\
$     system/lib64/libifx_jni.so \\
$     system/lib64/libossl3-crypto.so \\
$     system/lib64/libtss2-esys.so \\
$     system/lib64/libtss2-mu.so \\
$     system/lib64/libtss2-ossl3-provider.so \\
$     system/lib64/libtss2-rc.so \\
$     system/lib64/libtss2-sys.so \\
$     system/lib64/libtss2-tcti-mssim.so \\
$     system/lib64/libtss2-tctildr.so \\
$ EOF
```

Rebuild the AOSP:
```all
$ cd ~/aosp
$ m -j
```

## Running the Example Application

Start the emulator in headless mode:
```all
$ cd ~/aosp
$ source build/envsetup.sh
$ lunch sdk_phone_x86_64
$ export ANDROID_EMULATOR_WAIT_TIME_BEFORE_KILL=1

$ emulator -selinux permissive -no-window -no-audio &
$ EMULATOR_PID=$!
```

Wait until the emulator is ready for use:
```all
$ export PATH=${HOME}/aosp/out/host/linux-x86/bin:$PATH

# Usage: ./emulator_check.sh <emulator pid> <desired android version> <max wait time in seconds>
$ chmod +x ~/optiga-tpm-aosp/scripts/emulator_check.sh
$ ~/optiga-tpm-aosp/scripts/emulator_check.sh $EMULATOR_PID 14 120
```

Launch the application and monitor its activities for any errors:
```all
# Temporary Workaround for CI (GitHub Actions): As of now, efforts to implement a method for
# precisely determining when the Android emulator is ready to launch applications have
# been unsuccessful. As a makeshift solution, a delay has been introduced.
$ sleep 30

# Check if the application is installed
$ adb shell pm list packages -a | grep com.ifx.nave

# Clear the log
$ adb shell logcat -c

# Launch the application
$ adb shell am start -n com.ifx.nave/com.ifx.nave.MainActivity

# Usage: ./app_check.sh <max wait time in seconds>
$ chmod +x ~/optiga-tpm-aosp/scripts/app_check.sh
$ ~/optiga-tpm-aosp/scripts/app_check.sh 120
```

Terminate the application:
```all
$ adb shell am force-stop com.ifx.nave
```

Terminate the emulator:
```all
$ kill -SIGTERM $EMULATOR_PID
```

# License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
