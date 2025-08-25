{
  description = "A Hyprland plugin to elevate focused windows";

  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nix-filter.url = "github:numtide/nix-filter";
  };

  outputs = { self, hyprland, nix-filter, ... }:
    let
      inherit (hyprland.inputs) nixpkgs;
      forHyprlandSystems = fn: nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (system: fn system nixpkgs.legacyPackages.${system});
    in
    {
      packages = forHyprlandSystems
        (system: pkgs: rec {
          hyprlift = pkgs.gcc14Stdenv.mkDerivation { # Using gcc14Stdenv for compatibility
            pname = "hyprlift";
            version = "0.1";
            src = nix-filter.lib {
              root = ./.;
              include = [
                "src"
                ./Makefile
              ];
            };

            nativeBuildInputs = with pkgs; [ pkg-config ];

            # This is the key! It pulls in all of Hyprland's build dependencies automatically.
            buildInputs = [ hyprland.packages.${system}.hyprland.dev ] ++ hyprland.packages.${system}.hyprland.buildInputs;

            # The make command to build the plugin
            buildPhase = "make all";

            installPhase = ''
              mkdir -p $out/lib
              install ./hyprlift.so $out/lib/libhyprlift.so
            '';

            meta = with pkgs.lib; {
              homepage = "";
              description = "A plugin to elevate the focused window.";
              license = licenses.bsd3;
              platforms = platforms.linux;
            };
          };
          default = hyprlift;
        });
    };
}
