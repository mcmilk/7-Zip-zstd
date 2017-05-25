#!/bin/sh
# C/lizard/*
# /TR 2017-05-25

for i in *.c *.h; do
  sed -i 's/LZ5_/LIZ_/g' "$i"
  sed -i 's/LZ5F_/LIZF_/g' "$i"
  sed -i 's/"lz5_/"liz_/g' "$i"
done

for f in lz5*; do
  l=`echo $f|sed -e 's/lz5/liz/g'`
  mv $f $l
done
