{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.findmy-beacon;
  findmyPkg = pkgs.callPackage ../../Projects/findmy-linux/default.nix {};
in
{
  options.services.findmy-beacon = {
    enable = mkEnableOption "Apple Find My / OpenHaystack BLE offline tracking beacon";

    keyFile = mkOption {
      type = types.path;
      default = "/var/lib/findmy/advertisement.key";
      description = "Path to the base64 Apple Find My advertisement key file.";
    };

    interface = mkOption {
      type = types.int;
      default = 0;
      description = "HCI Bluetooth adapter ID (e.g. 0 for hci0).";
    };

    interval = mkOption {
      type = types.int;
      default = 2000;
      description = "Advertising pulse interval in milliseconds.";
    };
  };

  config = mkIf cfg.enable {
    # Ensure Bluetooth hardware subsystem is powered at boot
    hardware.bluetooth.enable = true;
    hardware.bluetooth.powerOnOnBoot = true;

    environment.systemPackages = [ findmyPkg ];

    systemd.services.findmy-beacon = {
      description = "FindMy Linux BLE Offline Beacon";
      wantedBy = [ "multi-user.target" ];
      after = [ "bluetooth.service" ];
      wants = [ "bluetooth.service" ];

      serviceConfig = {
        Type = "simple";
        ExecStart = "${findmyPkg}/bin/findmy-beacon --key-file ${toString cfg.keyFile} --interface ${toString cfg.interface} --interval ${toString cfg.interval}";
        Restart = "always";
        RestartSec = "5s";

        # Linux Capabilities
        CapabilityBoundingSet = [ "CAP_NET_ADMIN" "CAP_NET_RAW" ];
        AmbientCapabilities = [ "CAP_NET_ADMIN" "CAP_NET_RAW" ];
        NoNewPrivileges = true;
        ProtectSystem = "strict";
        ProtectHome = "read-only";
        PrivateTmp = true;
      };
    };
  };
}
