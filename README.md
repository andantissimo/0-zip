# 0-zip

converts and/or extracts data in *.pdf, *.rar or *.zip to a noncompressed zip archive.

<table>
<tr>
  <th>Format</th>
  <th>Records</th>
  <th>Order By</th>
</tr>
<tr>
  <td>PDF</td>
  <td>Embedded JPEGs, <s>Texts</s>, <s>Primitives</s></td>
  <td>Occurrence order</td>
</tr>
<tr>
  <td>RAR</td>
  <td rowspan="2">Non-protected entries</td>
  <td rowspan="2">Natural sort order</td>
</tr>
<tr>
  <td>ZIP</td>
</tr>
</table>

## Runtime Dependencies

* [UnRAR](https://www.rarlab.com/rar_add.htm) (Optional)

## Build Instructions

### Windows Requirements

* Visual Studio 2019+
  * Windows Universal CRT
* vcpkg
  * boost-crc:x64-windows-static
  * boost-interprocess:x64-windows-static
  * boost-iostreams:x64-windows-static
  * boost-locale:x64-windows-static
  * boost-program-options:x64-windows-static
  * zlib:x64-windows-static

### How to Build for Windows

1. open `msvc/0-zip.sln`
2. build `Release|x64`
3. copy `msvc/bin/x64/Release/0z.exe` to wherever you want

### POSIX Requirements

* CMake 3.8+
* Clang 10+ or GCC 10+ or Xcode 13+
* Boost 1.66+
  * boost-iostreams
  * boost-locale
  * boost-program-options
* zlib

### How to Build for POSIX

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```
