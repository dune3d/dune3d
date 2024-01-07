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
        pkgs.cmake
        pkgs.librsvg
        pkgs.gobject-introspection
        pkgs.pkg-config
        pkgs.wrapGAppsHook
        pythonEnv
      ];
      buildInputs = [
        pkgs.eigen
        pkgs.glm
        pkgs.gtkmm4
        pkgs.libepoxy
        pkgs.libspnav
        pkgs.libuuid
        pkgs.opencascade-occt
      ];
      env.CASROOT = pkgs.opencascade-occt;
    };
  };
}
