# Building Dune3D from source

## General Instructions

You're going to need these dependencies:

 - gtkmm-4.0
 - cairomm
 - opencascade
 - uuid
 - pkg-config (build-time only)
 - librsvg (build-time only)
 - glm (build-time only, since it's just headers)
 - python (build-time only)
 - cmake (build-time only)
 - meson (build-time only)
 - eigen  (build-time only, since it's just headers)
 - git (build-time only, required when building from a git repo)
 - python-gobject (build-time only)
 - python-cairo (build-time only)

And a C++ compiler that supports `format`.

Then run
```
meson setup build
meson compile -C build
```

This should work on any reasonably up-to-date Linux distro, various BSDs and Windows using MSYS2.

See [the CI configuration](../.github/workflows/all.yml) for the exact package names for debian-based distros and Arch Linux.

> [!IMPORTANT]
> Dune 3D is currently still alpha software undergoing rapid development, so please don't package it for
> your favourite distro yet. Users have expectations regarding stability and completeness towards packaged
> software that Dune 3D doesn't meet yet. Also having built it from source makes it easier to get the
> latest bugfixes and simplifies debugging.

## Building and running on NixOS (Linux x86_64)

To build and run the latest version of Dune3D, just use this command:

```
nix --experimental-features "nix-command flakes" run github:dune3d/dune3d
```

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
	mingw-w64-x86_64-eigen3 \
	mingw-w64-x86_64-meson \
	mingw-w64-x86_64-cmake \
	mingw-w64-x86_64-python \
	mingw-w64-x86_64-python-cairo \
	mingw-w64-x86_64-python-gobject \
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

> If you get the error `meson: command not found`, you are probably using the MSYS console (rather than the MINGW64 console).
> You can run the command `export PATH=/bin/mingw64:$PATH` to tell the console where to find meson (and the rest of mingw64).

### Running

```
./build/dune3d.exe
```

## Building on macOS

On macOS, homebrew, and LLVM from homebrew are needed to compile dune3d.

### Installing dependencies

[Homebrew](https://brew.sh/):
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the instructions, now you should be able to install the remaining dependencies:

```
brew install \
	python@3
	llvm \
	eigen \
	opencascade \
	pkg-config \
	gtk4 \
	gtkmm4 \
	glm \
	range-v3 \
	mimalloc \
	pygobject3 \
	librsvg
```

Install meson from source (at the time of writing this, the stable version has an issue that breaks the build on macOS):

```
brew install --HEAD meson
```

Now you can build the project using the `./scripts/build_macos.sh` script.

You should now have the `dune3d` executable in the `build/` folder.