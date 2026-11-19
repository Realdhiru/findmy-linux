{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "findmy-beacon";
  version = "1.0.0";

  src = ./.;

  buildPhase = ''
    runHook preBuild
    $CC -std=c99 -Wall -Wextra -O2 -D_GNU_SOURCE src/main.c src/hci.c src/beacon.c -o findmy-beacon
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    install -Dm755 findmy-beacon $out/bin/findmy-beacon
    install -Dm755 tools/keygen.sh $out/bin/findmy-keygen
    runHook postInstall
  '';

  meta = with pkgs.lib; {
    description = "Ultra-low-power native Apple Find My BLE beacon daemon for universal Linux";
    homepage = "https://github.com/realdhiru/findmy-linux";
    license = licenses.gpl3Plus;
    platforms = platforms.linux;
  };
}
