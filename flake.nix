{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

    # ESP-IDF overlay
    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      nixpkgs-esp-dev,
    }:
    let
      forSystems =
        systems: f:
        builtins.listToAttrs (
          map (system: {
            name = system;
            value = f system;
          }) systems
        );

      systems = [
        "x86_64-linux"
        "aarch64-darwin"
      ];
    in
    {
      devShells = forSystems systems (
        system:
        let
          # host specific pkgs
          pkgs = import nixpkgs { inherit system; };

          # ESP-IDF plus clangd
          idf = nixpkgs-esp-dev.packages.${system}.esp-idf-esp32.override (final: {
            toolsToInclude = final.toolsToInclude ++ [ "esp-clang" ];
          });

          clientShell = pkgs.mkShell {
            buildInputs = [ idf ];

            # Make clangd find the Xtensa tool-chain
            shellHook = ''
              export CLANGD_QUERY_DRIVER=${idf}/bin/xtensa-esp32-elf-g++
            '';
          };

          serverShell = pkgs.mkShell {
            buildInputs = with pkgs; [
              bear
              gdb
              pkg-config
              gnumake
              freetype
              opencv
              yaml-cpp
              libjpeg
              libpng
              xorg.libX11
              xorg.libXext
              xorg.libXrender
            ];
          };
        in
        {
          default = clientShell;
          client = clientShell;
          server = serverShell;
        }
      );
    };
}
