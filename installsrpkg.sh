#!/bin/bash

killall streamripperd
tar xvf srpkg.tar.gz
rsync -av srpkg/storage/ /storage/
rsync -av srpkg/ubuntuhome/ /home/rborisov/
rm -r srpkg
killall streamripperd
