/********************************************************************************************************************\
* Project name: Uncompressed Standard BMP to ASCII Art                                                               *
* Language    : C                                                                                                    *
* Comment     : I'll make it so that compressed BMP can also be converted to ASCII Art in the future                 *
*                   (the next week perhaps)                                                                          *                                                              *
*                                                                                                                    *
* Status      : Incomplete                                                                                           *
*                                                                                                                    *
* References:                                                                                                        *
*    THE BMP FILE FORMAT - University of Alberta                                                                     *
*        https://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm  *
*                                                                                                                    *
*    Microsoft Documentation                                                                                         *
*        https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader                       *
*        https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader                       *
*        https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-rgbquad                                *
*                                                                                                                    *
*    Online Forum:                                                                                                   *
*        Stack Overflow                                                                                              *
\********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "bmpLib.h"

#define CYAN "\x1B[36m"
#define NORM "\x1B[0m"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("\nFormat: %s <path of the BMP image> <path of the text file to write>\n", argv[0]);
        return 1;
    }

    FILE *BMP_FILE, *TEXT_FILE;
    if (!(BMP_FILE  = fopen(argv[1], "rb")) ||
        !(TEXT_FILE = fopen(argv[2], "wb"))) {

        if (BMP_FILE) fclose(BMP_FILE); // BMP_FILE must be opened if the error wasn't caused by it
        printf("\n%s: failed to open the %s file\n", strerror(errno), BMP_FILE ? "text" : "BMP");
        return 1;
    }

    BMP_FILE_HEADER F_H;
    BMP_INFO_HEADER I_H;

    if ((fread(&F_H, 1, sizeof(F_H), BMP_FILE) < sizeof(F_H) && !feof(BMP_FILE)) ||
        (fread(&I_H, 1, sizeof(I_H), BMP_FILE) < sizeof(I_H) && !feof(BMP_FILE))) {
        
        handleError("\nAn error has occurred when reading from the BMP file\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    if (F_H.FILE_TYPE != 0x4D42 || I_H.IMAGE_HEIGHT == 0 || I_H.IMAGE_WIDTH <= 0 || !bPP_TO_bPPs_INDEX(I_H.BITS_PER_PIXEL)) {
        // not 'BM',
        // height == 0,
        // width <= 0 (a negative height flips the image vertically but I'm not sure if a negative width does it too (I read the documentation))
        // bPP_TO_bPPs_INDEX() == 0 which means the the bPP of the BMP is not supported
        handleError("\nPlease use a standard BMP file\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    if (I_H.COMPRESSION) {
        handleError("\nPlease use an uncompressed BMP file\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    int pEntries = I_H.USED_COLOR_COUNT ? I_H.USED_COLOR_COUNT : 1 << I_H.BITS_PER_PIXEL;
    PALETTE* COLOR_TABLE = malloc(pEntries * sizeof(PALETTE));
    // 1 << n = pow(2, n)

    if (!COLOR_TABLE) {
        handleError("\nMemory allocation failed\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    if (I_H.BITS_PER_PIXEL <= 8) { // if bits per pixel <= 8, palette presents
        if (fread(COLOR_TABLE, sizeof(PALETTE), pEntries, BMP_FILE) < pEntries && !feof(BMP_FILE)) {
        
            handleError("\nAn error has occurred when reading from the BMP file\n", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE);
            return 1;
        }
    } else {
        // that means the PALETTE doesn't exist, so you free the memory
        free(COLOR_TABLE);
    }
    
    if (TO_ASCII(&F_H, &I_H, COLOR_TABLE, BMP_FILE, TEXT_FILE, bPPs[bPP_TO_bPPs_INDEX(I_H.BITS_PER_PIXEL) - 1])) {
        handleError("\nFailure\n", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE);
        return 1;
    }
    
    handleError("", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE); // closing files before exiting (not error handling)

    printf("\n- Success -\nPrinted the ASCII art to the file at %s%s%s\nImage Width: %s%d%s\nImage height: %s%d%s\nImage bPP: %s%d%s\n",
           CYAN, argv[2], NORM, CYAN, I_H.IMAGE_WIDTH, NORM, CYAN, I_H.IMAGE_HEIGHT, NORM, CYAN, I_H.BITS_PER_PIXEL, NORM);

    return 0;
}