# Any to Zip

converts *.pdf, *.rar and *.zip to noncompressed zip format.

* PDF - embedded JPEGs will be extracted and zipped, texts won't be rendered.
* RAR - will be converted to zip format, requires external library [UnRAR](https://www.rarlab.com/rar_add.htm).
* ZIP - entries will be sorted and uncompressed.

## Windows Requirements
* Visual Studio 2019+
  * Windows Universal CRT
* VCpkg
  * boost-crc:x64-windows-static
  * boost-interprocess:x64-windows-static
  * boost-locale:x64-windows-static
  * boost-program-options:x64-windows-static

## Generic Requirements
* CMake 3.1+
* Clang 6.0+ or GCC 7.0+ or Xcode 10.0+
* Boost 1.66+ (boost-locale, boost-program-options)

## Windows Installation
1. open msvc/a-z.sln
2. build Release x64
3. copy msvc/bin/x64/Release/a-z.exe to wherever you want

## Generic Installation
```sh
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ sudo make install
```
