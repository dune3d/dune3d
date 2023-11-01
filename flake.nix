{
  description = "Dune 3D CAD application";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      inherit system;
    };
  in {
    packages."${system}".default = let
      stdenv = pkgs.gcc13Stdenv;
      pythonEnv = pkgs.python3.withPackages(p: [ p.pygobject3 ]);
    in stdenv.mkDerivation rec {
      name = "dune3d";
      src = ./.;
      nativeBuildInputs = [
        pkgs.meson
        pkgs.ninja
        pkgs.gobject-introspection
        pkgs.pkg-config
        pkgs.wrapGAppsHook
        pythonEnv
      ];
      buildInputs = [
        pkgs.cmake
        pkgs.eigen
        pkgs.glm
        pkgs.gtkmm4
        pkgs.libepoxy
        pkgs.librsvg
        pkgs.libuuid
        pkgs.mimalloc
        pkgs.opencascade-occt
        pkgs.range-v3
      ];
      env.CASROOT = pkgs.opencascade-occt;
    };
  };
}
