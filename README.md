
# BMP Image to ASCII Art

## Description
it turns **uncompressed BMP** to **ASCII arts**
it works with **uncompressed BMP files** which have bit depths *(bits per pixel)* of:
1 (untested),
4 (tested),
8 (tested),
16 (untested),
24 (tested),
32 (untested)

## Compiling:
    gcc bmp.c bmpLib.c -o bmp -lm
