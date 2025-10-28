#!/bin/bash
mkdir _build
cd _build
cmake .. -DUSEUCD=no -DUSEWX=no -DCOLORER=no
make -j$(nproc --all)
