/********************************************************************************************************************\
* Project name: Uncompressed Standard BMP to ASCII Art                                                               *
* Language    : C                                                                                                    *
* Comment     : I'll make it so that compressed BMP can also be converted to ASCII Art in the future                 *
*               TODO: include 1, 4, 8, 16, and 32 BPP                                                                *
* Status      : Incomplete                                                                                           *
*                                                                                                                    *
* References:                                                                                                        *
*    THE BMP FILE FORMAT - University of Alberta                                                                     *
*        https://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm  *
*                                                                                                                    *
*    HW11 – ECE 264 Advanced C Programming – Spring 2019                                                             *
*        https://engineering.purdue.edu/ece264/19sp/hw/HW11                                                          *
*                                                                                                                    *
*    Online Forum:                                                                                                   *
*        Stack Overflow                                                                                              *
\********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>

typedef struct __attribute__((packed)) {
    unsigned short int FILE_TYPE;
    unsigned int       FILE_SIZE;
    unsigned short int RESERVED0;
    unsigned short int RESERVED1;
    unsigned int       DATA_OFFSET;
} BMP_FILE_HEADER;

typedef struct __attribute__((packed)) {
    unsigned int       FILE_HEADER_SIZE;
    signed int         IMAGE_WIDTH;
    signed int         IMAGE_HEIGHT;
    unsigned short int COLOR_PLANE_COUNT;
    unsigned short int BITS_PER_PIXEL;
    unsigned int       COMPRESSION;
    unsigned int       IMAGE_SIZE;
    unsigned int       X_RESOLUTION_PPM; // PPM = Pixel Per Meter
    unsigned int       Y_RESOLUTION_PPM;
    unsigned int       USED_COLOR_COUNT;
    unsigned int       IMPORTANT_COLOR_COUNT;
} BMP_INFO_HEADER;

const char CHARS[] = " .-_!()^*%&#$@";

void handleError(const char* string, int nFile, int nAlloc, int total, ...) {
/*****************************************************************************************************************\
* + string: the message to be printed                                                                             *
* + nFile : the count of files to be closed                                                                       *
* + nAlloc: the count of allocated memory to free                                                                 *
* + total : nFile + nAlloc (the second parameter of va_start() must be the last fixed parameter of this function) *
\*****************************************************************************************************************/
    va_list items;
    va_start(items, total);

    for (int i = 0; i < nFile; i++) {
        FILE* file = va_arg(items, FILE*);
        if (file) {
            fclose(file);
            file = NULL;
        }
    }

    for (int i = nFile; i < nAlloc; i++) {
        void* ptr = va_arg(items, void*);
        if (ptr) {
            free(ptr);
            ptr = NULL;
        }
    }

    va_end(items);

    printf("%s", string);
}

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
        
        handleError("\nAn error has occurred when reading from file(s)\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    if (F_H.FILE_TYPE != 0x4D42) {
        handleError("\nThe file is not of type \"BMP\"\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    if (I_H.COMPRESSION) {
        handleError("\nPlease use an uncompressed BMP file\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    int bitsPerRow = (int) I_H.BITS_PER_PIXEL * I_H.IMAGE_WIDTH;
    int bytesPerRow = (bitsPerRow + (bitsPerRow % 32 == 0 ? 0 : (32 - bitsPerRow % 32))) / 8; // ((bits per row + the padding) / 8) = bytes per row

    if (fseek(BMP_FILE, F_H.DATA_OFFSET + (I_H.IMAGE_HEIGHT < 0 ? 0 : bytesPerRow * (I_H.IMAGE_HEIGHT - 1)), SEEK_SET)) {
        handleError("\nUnable to move file pointer\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    unsigned char* row = malloc(bytesPerRow);
    if (!row) {
        handleError("\nUnable to allocate memory for reading\n", 2, 0, 2, BMP_FILE, TEXT_FILE);
        return 1;
    }

    for (int y = 0; y < I_H.IMAGE_HEIGHT; y++) {
        /*****                                                                                   *****\
        * BMP pixel data is stored bottom-up (from the lower part of the image to the upper part)...  *
        * unless the height is negative                                                               *
        \*****                                                                                   *****/
        
        if (fread(row, 1, bytesPerRow, BMP_FILE) < bytesPerRow && !feof(BMP_FILE)) {
            handleError("\nUnable to read data from the BMP file\n", 2, 1, 3, BMP_FILE, TEXT_FILE, (void*) row);
            return 1;
        }

        for (int x = 0; x < I_H.IMAGE_WIDTH; x++) {
            int index = (int) round((double) ((row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2]) / 765.0 * (strlen(CHARS) - 1)));

            if (fputc(CHARS[index], TEXT_FILE) == EOF) {
                handleError("\nUnable to write into the text file\n", 2, 1, 3, BMP_FILE, TEXT_FILE, (void*) row);
                return 1;
            }
        }

        fputc('\n', TEXT_FILE);
        if (I_H.IMAGE_HEIGHT >= 0) fseek(BMP_FILE, -2 * bytesPerRow, SEEK_CUR);
    }

    handleError("", 2, 1, 3, BMP_FILE, TEXT_FILE, (void*) row); // closing files and freeing allocated memory before exit (not error handling)

    printf("\nSuccess\n");

    return 0;
}