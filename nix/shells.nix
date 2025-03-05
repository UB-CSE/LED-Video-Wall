{ pkgs, ... }:
let
  serverShell = pkgs.mkShell {
    buildInputs =
      with pkgs;
      [
        bear
        gdb
      ]
      ++ pkgs.packages.server.buildInputs
      ++ pkgs.packages.server.nativeBuildInputs;

    shellHook = pkgs.packages.server.shellHook;
  };
in
{
  default = serverShell;
}
