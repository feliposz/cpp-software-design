# Software Design By Example

My implementation of the code and exercises from the book [Software Design By Example](https://third-bit.com/sdxpy/).

Instead of using Python, I've done it in ~~(very bad)~~ C++.

## OpenSSL Installation

[vcpkg](https://vcpkg.io/en/) installed with:

	git clone https://github.com/microsoft/vcpkg.git
	cd vcpkg
	bootstrap-vcpkg.bat

[OpenSSL](https://www.openssl.org/) installed with:

	vcpkg install openssl-windows --triplet x64-windows

## Configuring OpenSSL

In *Project Properties -> C/C++ -> General -> Additional Include Directories*:

	...\vcpkg\packages\openssl_x64-windows\include

In *Project Properties -> Linker -> General -> Additional Library Directories*:

	...\vcpkg\packages\openssl_x64-windows\lib

In *Project Properties -> Linker -> Input -> Additional Dependencies*

	libcrypto.lib
	libssl.lib

In *Project Properties -> Debugging > Environment*:

	PATH=%PATH%;...\vcpkg\packages\openssl_x64-windows\bin\

## nlohmann's JSON library

Installation:

  vcpkg install nlohmann-json

In *Project Properties -> C/C++ -> General -> Additional Include Directories*:

  ...\vcpkg\packages\nlohmann-json_x64-windows\include

## LibXML

Installation:

  vcpkg install libxml2

In *Project Properties -> C/C++ -> General -> Additional Include Directories*:

  ...\vcpkg\packages\libxml2_x64-windows\include
  ...\vcpkg\packages\libiconv_x64-windows\include

In *Project Properties -> Linker -> General -> Additional Library Directories*:

	...\vcpkg\packages\libxml2_x64-windows\lib

In *Project Properties -> Linker -> Input -> Additional Dependencies*

	libxml2.lib

In *Project Properties -> Debugging > Environment*:

	PATH=%PATH%;...\vcpkg\packages\openssl_x64-windows\bin\;...\vcpkg\packages\liblzma_x64-windows\bin;...\vcpkg\packages\zlib_x64-windows\bin\zlib1.dll;...\vcpkg\packages\libiconv_x64-windows\bin


## References:

- https://stackoverflow.com/questions/32156336/how-to-include-openssl-in-visual-studio
- https://stackoverflow.com/questions/31586701/generate-sha256-in-c
- https://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c/10632725
