
#let
#  #nixpkgs = import <nixpkgs>;
#  nixpkgs = import ./nixpkgs.nix;
#  pkgs = import nixpkgs {
#    config = {};
#    overlays = [
#      (import ./overlay.nix)
#    ];
#  };
#
#in pkgs.ygor


let pkgs = import <nixpkgs> {}; 
in pkgs.callPackage( (import ./testcompile_derivation.nix) ){ 
   ygor = pkgs.callPackage ./ygor_derivation.nix { };
   # other_custom_dep = ...
   # other_custom_dep = ...
   # ...
}

