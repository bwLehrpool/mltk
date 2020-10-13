#!/bin/bash

mkdir -p out/active out/inactive

for i in active/*.png; do
	convert "$i" -background '#ffffff' -alpha Background \
		-modulate 120,70 \
		-alpha Off -resize 64x64 "out/${i%.png}.ppm"
done

for i in inactive/*.png; do
	convert "$i" -background '#ffffff' -alpha Background \
		-colorspace Gray -modulate 180 \
		-alpha Off -resize 64x64 "out/${i%.png}.ppm"
done

