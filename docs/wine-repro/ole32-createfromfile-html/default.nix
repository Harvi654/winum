# Builds the MEASURE programs (x86_64, mingw-w64).
#   nix-build docs/wine-repro/ole32-createfromfile-html
{
  pkgs ? import <nixpkgs> { },
}:
pkgs.pkgsCross.mingwW64.stdenv.mkDerivation {
  name = "ole32-createfromfile-html";
  src = ./.;
  buildPhase = ''
    $CC -O2 -Wall -static-libgcc -o createfromfile.exe createfromfile.c -lole32 -luuid
    $CC -O2 -Wall -static-libgcc -municode -o stglist.exe stglist.c -lole32 -luuid
    $CC -O2 -Wall -static-libgcc -municode -o stgerr.exe stgerr.c -lole32 -luuid
    $CC -O2 -Wall -static-libgcc -municode -o propnull.exe propnull.c -lole32 -luuid
    $CC -O2 -Wall -static-libgcc -municode -o pkgprobe.exe pkgprobe.c -lole32 -luuid -lgdi32 -lshell32
  '';
  installPhase = "install -D -t $out createfromfile.exe stglist.exe stgerr.exe propnull.exe pkgprobe.exe";
}
