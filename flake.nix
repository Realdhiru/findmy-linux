{
  description = "Universal, low-power Apple Find My BLE beacon daemon for Linux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs { inherit system; };
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = self.packages.${system}.findmy-beacon;
          findmy-beacon = pkgs.stdenv.mkDerivation {
            pname = "findmy-beacon";
            version = "1.0.0";
            src = ./.;

            buildInputs = [ pkgs.openssl ];

            buildPhase = ''
              runHook preBuild
              $CC -std=c99 -Wall -Wextra -O2 -D_GNU_SOURCE src/main.c src/hci.c src/beacon.c -o findmy-beacon
              $CC -std=c99 -Wall -Wextra -O2 -D_GNU_SOURCE src/keygen.c src/beacon.c -lcrypto -o findmy-keygen
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              install -Dm755 findmy-beacon $out/bin/findmy-beacon
              install -Dm755 findmy-keygen $out/bin/findmy-keygen
              runHook postInstall
            '';

            meta = with pkgs.lib; {
              description = "Universal Apple Find My BLE beacon daemon for Linux";
              homepage = "https://github.com/realdhiru/findmy-linux";
              license = licenses.gpl3Plus;
              platforms = platforms.linux;
              mainProgram = "findmy-beacon";
            };
          };
        }
      );

      nixosModules.default = { config, lib, pkgs, ... }:
        let
          cfg = config.services.findmy-beacon;
          pkg = self.packages.${pkgs.stdenv.hostPlatform.system}.findmy-beacon;
        in
        {
          options.services.findmy-beacon = {
            enable = lib.mkEnableOption "Apple Find My offline BLE tracking beacon";
            keyFile = lib.mkOption {
              type = lib.types.path;
              default = "/var/lib/findmy/advertisement.key";
              description = "Path to base64 advertisement key file.";
            };
            interval = lib.mkOption {
              type = lib.types.int;
              default = 2000;
              description = "Advertising interval in milliseconds.";
            };
          };

          config = lib.mkIf cfg.enable {
            hardware.bluetooth.enable = true;
            environment.systemPackages = [ pkg ];

            systemd.services.findmy-beacon = {
              description = "Apple Find My Offline BLE Tracking Beacon";
              wantedBy = [ "multi-user.target" ];
              after = [ "bluetooth.service" ];
              wants = [ "bluetooth.service" ];

              serviceConfig = {
                Type = "simple";
                ExecStart = "${pkg}/bin/findmy-beacon -f ${toString cfg.keyFile} -t ${toString cfg.interval}";
                Restart = "on-failure";
                RestartSec = "5s";
                CapabilityBoundingSet = [ "CAP_NET_ADMIN" "CAP_NET_RAW" ];
                AmbientCapabilities = [ "CAP_NET_ADMIN" "CAP_NET_RAW" ];
                NoNewPrivileges = true;
                ProtectSystem = "strict";
                ProtectHome = "read-only";
                PrivateTmp = true;
              };
            };
          };
        };
    };
}
