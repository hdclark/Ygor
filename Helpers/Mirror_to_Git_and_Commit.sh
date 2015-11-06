#!/bin/sh

rsync -avz '/home/hal/Dropbox/Project - Ygor/' '/home/hal/Development/Project - Ygor/'

pushd .
cd '/home/hal/Development/Project - Ygor/'
git commit -a
popd 

