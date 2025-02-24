{
  description = "LED Video Wall flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    arduino-nix.url = "github:bouk/arduino-nix";
    arduino-index = {
      url = "github:bouk/arduino-indexes";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      arduino-nix,
      arduino-index,
      ...
    }:
    let
      overlays = import ./nix/overlays.nix { inherit arduino-nix arduino-index; };
    in
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system overlays; };
      in
      {
        nixConfig.substituters = [ ];

        packages = import ./nix/packages.nix { inherit pkgs; };
        devShells = import ./nix/shells.nix { inherit pkgs; };
        apps = import ./nix/apps.nix {
          inherit pkgs flake-utils;
          packages = self.packages.${system};
        };
      }
    );
}
