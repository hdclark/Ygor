
{ stdenv
, ygor
, ...
}:

#with import <nixpkgs> {};

stdenv.mkDerivation rec {
  pname = "testcompile";
  version = "2";

  #src = [ ];
  phases = [ "buildPhase"
             "installPhase"
             "fixupPhase" 
  ];

  buildInputs = [ 
    ygor
  ];

  buildPhase = ''
    echo '#include <YgorMisc.h>' > in.cc
    echo 'int main(int argc, char **argv){ return 0; }' >> in.cc
    $CXX -std=c++17 -Wall -Wextra -pedantic -o testcompile in.cc -lygor
  '';

  installPhase = ''
    mkdir -p $out/bin/
    cp testcompile $out/bin/
  '';

}

