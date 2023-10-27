# Building Dune3D from source

## General Instructions

You're going to need these dependencies:

 - gtkmm4
 - libepoxy
 - eigen
 - opencascade (oce doesn't appear to compile)
 - mimalloc
 - glm
 - range-v3

Then run
```
meson setup build
meson compile -C build
```

This should work on any reasonably up-to-date Linux distro, various BSDs and Windows using MSYS2.

See [the CI configuration](../.github/workflows/all.yml) for the exact package names for debian-based distros and Arch Linux.

See [below](#building-on-windows) for how to build on Windows.

> [!IMPORTANT]
> Dune 3D is currently still alpha software undergoing rapid development, so please don't package it for
> your favourite distro yet. Users have expecations regarding stability and completeness towards packaged
> software that Dune 3D doesn't meet yet. Also having built it from source makes it easier to get the
> latest bugfixes and simplifies debugging.


## Building on Windows

### Install MSYS2

Download and run the msys2 installer from http://msys2.github.io/

It is probably a good idea to pick the 64bit version and to make sure that the path you select for installation doesn’t contain any spaces.

### Start MSYS console

Launch the Start Menu item “MSYS2 MINGW64”, you should be greeted with a console window. All steps below refer to what you should type into that window.


### Install updates

```
pacman -Syu
```

if it tells you to close restart msys, close the console window and start it again. Then run `pacman -Syu` again.

### Install dependencies

```
pacman -S \
	mingw-w64-x86_64-gcc \
	mingw-w64-x86_64-pkgconf \
	mingw-w64-x86_64-gtkmm4 \
	mingw-w64-x86_64-glm \
	mingw-w64-x86_64-opencascade \
	mingw-w64-x86_64-mimalloc \
	mingw-w64-x86_64-eigen3 \
	mingw-w64-x86_64-meson \
	mingw-w64-x86_64-cmake \
	mingw-w64-x86_64-python \
	mingw-w64-x86_64-python-cairo \
	mingw-w64-x86_64-python-gobject \
	mingw-w64-x86_64-range-v3 \
	zip \
	unzip \
	git \
	dos2unix \
	--needed
```

### Clone Dune3D

```
git clone https://github.com/dune3d/dune3d
cd dune3d
```

### Build it

```
meson setup build
meson compile -C build
```

### Running

```
./build/dune3d.exe
```
