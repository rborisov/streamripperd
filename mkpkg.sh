#!/bin/bash

mkdir -p srpkg/storage/ca-st-ro/bin

cp -r etc/* srpkg/
cp build/streamripperd srpkg/storage/ca-st-ro/bin/

tar czf srpkg.tar.gz srpkg
