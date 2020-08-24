#!/usr/bin/env bash

set -eu

wget -q 'https://github.com/rmartinho/nonius/archive/v1.2.0-beta.1.tar.gz' -O nonius.tgz

tar -axf nonius.tgz
rm nonius.tgz

g++ -std=c++17 -Wall -I nonius-1.2.0-beta.1/include/ *.cc -o run_benchmarks -lygor -pthread

rm -rf nonius-1.2.0-beta.1/

./run_benchmarks -v -s 500

rm ./run_benchmarks

