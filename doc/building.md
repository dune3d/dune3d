# Building on Windows

## Install MSYS2

Download and run the msys2 installer from http://msys2.github.io/

It is probably a good idea to pick the 64bit version and to make sure that the path you select for installation doesn’t contain any spaces.

## Start MSYS console

Launch the Start Menu item “MSYS2 mingw 64 bit” you should be greeted with a console window. All steps below refer to what you should type into that window.
Install updates

```
pacman -Syu
```

if it tells you to close restart msys, close the console window and start it again. Then run `pacman -Syu` again.
Install dependencies

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

## Clone Dune3D

```
git clone https://github.com/dune3d/dune3d
cd dune3d
```

## Build it

```
meson setup build
meson compile -C build
```

## Running

```
build/dune3d.exe
```
