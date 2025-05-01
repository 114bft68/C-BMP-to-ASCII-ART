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
#include <inttypes.h>
#include "bmpLib.h"

#define CYAN "\x1B[36m"
#define NORM "\x1B[0m"

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("\nFormat: %s <path of the BMP image> <path of the text file to write>\n", argv[0]);
        return 1;
    }

    FILE *BMP_FILE  = fopen(argv[1], "rb"),
         *TEXT_FILE = NULL;

    if (!BMP_FILE || !(TEXT_FILE = fopen(argv[2], "wb")))
        safer_exit("\nOne of the files cannot be opened\n", 1, 0, 1, BMP_FILE); // 2 situations: BMP opened, TEXT failed; BMP failed

    BMP_FILE_HEADER F_H;
    BMP_INFO_HEADER I_H;

    if ((fread(&F_H, 1, sizeof(F_H), BMP_FILE) < sizeof(F_H) && !feof(BMP_FILE)) ||
        (fread(&I_H, 1, sizeof(I_H), BMP_FILE) < sizeof(I_H) && !feof(BMP_FILE)))
    {
        safer_exit("\nAn error has occurred when reading from the BMP file\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
    }

    if (!isSupported(&F_H, &I_H)) safer_exit("\nThe file you entered is not supported\n", 2, 0, 2, BMP_FILE, TEXT_FILE);

    PALETTE *COLOR_TABLE = NULL;

    if (I_H.BITS_PER_PIXEL <= 8) // if bits per pixel <= 8, palette presents   
    { 
        uint32_t pEntries = I_H.USED_COLOR_COUNT ? I_H.USED_COLOR_COUNT : 1 << I_H.BITS_PER_PIXEL;
        // 1 << n = pow(2, n)

        if (!(COLOR_TABLE = malloc(pEntries * sizeof(PALETTE)))) 
            safer_exit("\nMemory allocation failed\n", 2, 0, 2, BMP_FILE, TEXT_FILE);

        if (fread(COLOR_TABLE, sizeof(PALETTE), pEntries, BMP_FILE) < pEntries && !feof(BMP_FILE))
            safer_exit("\nAn error has occurred when reading from the BMP file\n", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE);
    }
    
    if (TO_ASCII(&F_H, &I_H, COLOR_TABLE, BMP_FILE, TEXT_FILE, bPPs[bPP_TO_bPPs_INDEX(I_H.BITS_PER_PIXEL) - 1]))
        safer_exit("\nFailure\n", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE);
    
    
    cleanup("", 2, 1, 3, BMP_FILE, TEXT_FILE, COLOR_TABLE); // close files and free COLOR_TABLE if it's not NULL

    printf("\n- Success -\nPrinted the ASCII art to the file at %s%s%s\nImage Width: %s%"
           PRId32
           "%s\nImage height: %s%"
           PRId32
           "%s\nImage bPP: %s%"
           PRIu16
           "%s\n",
           CYAN, argv[2], NORM, CYAN, I_H.IMAGE_WIDTH, NORM, CYAN, I_H.IMAGE_HEIGHT, NORM, CYAN, I_H.BITS_PER_PIXEL, NORM);

    return 0;
}