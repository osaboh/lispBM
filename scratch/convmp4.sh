#!/bin/bash


for f in *.raw
do
   res=${f%.raw}.png
   convert -depth 8 -size 32x32+0 rgb:$f -scale 512x512 $res 
done


ffmpeg -framerate 20 -pattern_type glob -i '*.png' -c:v libx264 -r 30 -pix_fmt yuv420p out.mp4
