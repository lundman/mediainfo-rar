
mediainfo-rar v1.4.0 is built against:

* MediaInfo_CLI_0.7.65_GNU_FromSource
* libdvdread-4.2.0plus
* libdvdnav-4.2.1
* unrar-3.7.8-seek
* undvd from llink-2.3.2


Welcome to mediainfo-rar utility, which attempts to simulate "mediainfo" exactly, with the addition of being able to parse files contained in the RAR archiver format.

Jorgen Lundman <lundman@lundman.net>

With thanks to MediaInfo developer Jérôme Martinez.


ENV

$MEDIAINFO_UNRAR can be set to the path of the unrar program. Note a
special unrar is required for mediainfo-rar to function correctly. It
needs to have the "-sk" option (seek to byte offset).

If unset, it will attempt to run "unrar" from current working directory.




WIN32

If you are building on Windows using the DevStudio project files, you
should download the Windows DLL without-installer, and unpack in the
win32 directory.

The path expected by the project files is:
win32/Developers/Source/MediaInfoDLL/

Typically, I would run the compiled versions from win32/ directory so
it can find "mediainfo.dll".


Unix build:

You will need the following source packages, the same or newer
versions:

MediaInfo_CLI_0.7.65_GNU_FromSource.tar.bz2

# tar -xf MediaInfo_CLI_0.7.65_GNU_FromSource.tar.bz2
# cd MediaInfo_CLI_GNU_FromSource

# cd ZenLib/Project/GNU/Library
# ./configure

OSX for UB 32/64 bit builds

# ./configure --disable-dependency-tracking
--with-macosx-version-min=`xcrun --show-sdk-version`
--with-macosx-sdk=`xcrun --show-sdk-path`  CPPFLAGS="-I`xcrun
--show-sdk-path`/usr/include" CFLAGS="-arch x86_64" LDFLAGS="-arch
x86_64" CXXFLAGS="-arch x86_64" --enable-arch-ppc=no
--enable-arch-ppc64=no --enable-arch-i386 --enable-arch-x86_64

# make
# sudo make install

# cd ../../../../

# cd MediaInfoLib/Project/GNU/Library

# ./configure

OSX

# ./configure --disable-dependency-tracking
  --with-macosx-version-min=`xcrun --show-sdk-version`
  --with-macosx-sdk=`xcrun --show-sdk-path`  CPPFLAGS="-I`xcrun
  --show-sdk-path`/usr/include" --enable-arch-ppc=no
  --enable-arch-ppc64=no --enable-arch-i386 --enable-arch-x86_64


# make
# sudo make install


Get libdvdread-4.2.0.plus

# cd libdvdread-4.2.0.plus
# ./configure
OSX
./configure "CFLAGS=-arch i386 -arch x86_64  -isysroot `xcrun
# --show-sdk-path`" LDFLAGS="-arch i386 -arch x86_64"
# --disable-dependency-tracking --enable-static

# make
# sudo make install



Get libdvdnav-4.2.1.tar.xz

# tar -xf libdvdnav-4.2.1.tar.xz
# cd libdvdnav-4.2.1
# ./autogen.sh
# ./configure "CFLAGS=-arch i386 -arch x86_64  -isysroot `xcrun
# --show-sdk-path`" LDFLAGS="-arch i386 -arch x86_64"
# --disable-dependency-tracking --enable-static
# make
# make install



mediainfo-rar

# cd mediainfo-rar

# ./configure "CFLAGS=-arch i386 -arch x86_64  -isysroot `xcrun
--show-sdk-path`" LDFLAGS="-arch i386 -arch x86_64 -L/usr/local/lib/"
--disable-dependency-tracking --enable-static
CPPFLAGS=-I/usr/local/include --enable-mediainfo-static

# make




Usage:

Listing RAR contents:

# mediainfo-rar -l directory/made-tsits.rar
-rw-r--r--   4640139264 made-tsits.iso


Pick first file in RAR:

# mediainfo-rar -l directory/made-tsits.rar


Specify filename inside RAR:

# mediainfo-rar  directory/made-tsits.rar/made-tsits.iso


Requires separate executables "unrar" and "undvd". "unrar" is special
version with seek option -sk enabled.



