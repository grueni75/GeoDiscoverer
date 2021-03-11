#!/bin/bash
gdalwarp \
    -of GTiff \
    -dstnodata 0 \
    -t_srs "EPSG:3857" \
    -r "cubic" \
    -multi \
    -co "TILED=YES" \
    -ts 32768 0 \
    "index.vrt" \
    "lowres.tif"

