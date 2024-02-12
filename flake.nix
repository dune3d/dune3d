{
  description = "Dune 3D CAD application";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        stdenv = if pkgs.stdenv.hostPlatform.isDarwin then pkgs.llvmPackages_17.stdenv
                  else pkgs.gcc13Stdenv;
        pythonEnv = pkgs.python3.withPackages (p: [ p.pygobject3 ]);
      in
      {
        packages = rec {
          dune3d =
            stdenv.mkDerivation {
              name = "dune3d";
              src = builtins.path { path = ./.; name = "dune3d-source"; };

              nativeBuildInputs = [
                pkgs.meson
                pkgs.ninja
                pkgs.cmake
                pkgs.librsvg
                pkgs.gobject-introspection
                pkgs.pkg-config
                pkgs.wrapGAppsHook
                pythonEnv
              ] ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isDarwin [
                pkgs.desktopToDarwinBundle
              ];

              buildInputs = [
                pkgs.eigen
                pkgs.glm
                pkgs.gtkmm4
                pkgs.libepoxy
                pkgs.libspnav
                (if pkgs.hostPlatform.isLinux then pkgs.libuuid else pkgs.libossp_uuid)
                pkgs.opencascade-occt
              ];

              meta = with pkgs.lib; {
                description = "Parametric 3D CAD";
                homepage = "https://dune3d.org";
                license = licenses.gpl3Only;
                platforms = platforms.unix;
                mainProgram = "dune3d";
              };
            };
          default = dune3d;
        };
        apps = rec {
          dune3d = flake-utils.lib.mkApp { drv = self.packages.${system}.dune3d; };
          default = dune3d;
        };
      }
    );
}
