{
  description = "LED Video Wall flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:

    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
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
