
let
  #nixpkgs = import <nixpkgs>;
  nixpkgs = import ./nixpkgs.nix;
  pkgs = import nixpkgs {
    config = {};
    overlays = [
      (import ./overlay.nix)
    ];
  };

in pkgs.ygor

