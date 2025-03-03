#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "bmpLib.h"

const char CHARS[] = " .-_!()^*%&#$@";
//const char CHARS[] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
#define CHARS_LEN strlen(CHARS)

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

int TO_ASCII(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH, PALETTE* COLOR_TABLE,
             FILE* BMP_FILE, FILE* TEXT_FILE,
             int (*f)(int, signed int, unsigned char*, PALETTE*, FILE*)) {

    int BPR_NO_PADDING = BIH->BITS_PER_PIXEL * BIH->IMAGE_WIDTH; // bits per row without padding
    
    if (BPR_NO_PADDING < 8) {
        printf("\nThe BMP image is too small\n");
        return 1;
        // this is way too small (not impossible to turn it into an ASCII art but it requires some efforts...)
        // you need to determine if BPR_NO_PADDING is lower than 8 then implement the loop inside BPP_(1, 4, 8, 16, 24, 32) so that the step is 1 bit
        // but this type of images are not common and wouldn't generate a good-looking ASCII art
    }

    int bitsPerRow  = BPR_NO_PADDING + (BPR_NO_PADDING % 32 == 0 ? 0 : (32 - BPR_NO_PADDING % 32)); // bits per row + paddin
    int bytesPerRow = bitsPerRow / 8;

    if (fseek(BMP_FILE, BFH->DATA_OFFSET + (BIH->IMAGE_HEIGHT < 0 ? 0 : bytesPerRow * (BIH->IMAGE_HEIGHT - 1)), SEEK_SET)) {
        perror("\nUnable to move file pointer");
        return 1;
    }

    unsigned char* row = malloc(bytesPerRow);
    if (!row) {
        printf("\nUnable to allocate memory for reading\n");
        return 1;
    }

    for (int y = 0; y < abs(BIH->IMAGE_HEIGHT); y++) {
        /*****                                                                                   *****\
        * BMP pixel data is stored bottom-up (from the lower part of the image to the upper part)...  *
        * unless the height is negative                                                               *
        \*****                                                                                   *****/
        
        if (fread(row, 1, bytesPerRow, BMP_FILE) < bytesPerRow && !feof(BMP_FILE)) {
            handleError("\nUnable to read data from the BMP file\n", 0, 1, 1, (void*) row);
            return 1;
        }

        if (f(BPR_NO_PADDING, BIH->IMAGE_WIDTH, row, COLOR_TABLE, TEXT_FILE)) {
            handleError("\nUnable to write into the text file\n", 0, 1, 1, (void*) row);
        }

        fputc('\n', TEXT_FILE);

        if (BIH->IMAGE_HEIGHT > 0 && y != abs(BIH->IMAGE_HEIGHT) - 1 &&
            fseek(BMP_FILE, -2 * bytesPerRow, SEEK_CUR))
        {
            /*
            fseek returns non 0 on failure,
            fseek will only be run if it isn't the last loop (no point for fseeking because no further data reading is needed) and
            BIH->IMAGE_HEIGHT is larger than 0 which means the data is stored bottom-up
            */
            handleError("\nUnable to move file pointer for reading\n", 0, 1, 1, (void*) row);
        }
    }

    handleError("", 0, 1, 1, (void*) row); // not error handling but memory freeing before exiting
    return 0;
}

// I honestly think BPP_1 and BPP_4 can both be shortened, however I'm too tired to think of a solution, next time maybe...?

int BPP_1(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 bit = 1 PALETTE
    // bits per row without padding / 8 (truncated if BPR / 8 is a float)
    for (int x = 0; x < (int) (BPR / 8); x++) {

        for (int i = 7; i >= 0; i--) {
            
            PALETTE BGR = COLOR_TABLE[row[x] & (1 << i) > 0 ? 1 : 0];
            if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

        }
    }

    int remainder = BPR % 8;
    if (remainder) {

        for (int i = 7; i >= remainder; i--) {

            PALETTE BGR = COLOR_TABLE[row[(((int) (BPR / 8)) & (1 << i)) > 0 ? 1 : 0]];
            if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
        }
    }
    
    return 0;
}

int BPP_4(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // each of the first 4 bits and the last 4 bits represent a PALETTE

    for (int x = 0; x < (int) (BPR / 8); x++) {

        PALETTE entries[] = {
            COLOR_TABLE[row[x] >> 4],
            COLOR_TABLE[row[x] & 0b00001111]
        };
    
        for (int i = 0; i < 2; i++) {
            if (fputc(CHARS[(int) round((double) ((entries[i].BLUE + entries[i].GREEN + entries[i].RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) 
                return 1;
        }

    }

    int remainder = BPR % 8;
    if (remainder) { // 4 bits for 1 pixel

        PALETTE BGR = COLOR_TABLE[row[((int) (BPR / 8)) >> 4]];
        if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
    }
    
    return 0;
}

int BPP_8(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 byte = 1 PALETTE
    for (int x = 0; x < WIDTH; x++) {
    
        PALETTE BGR = COLOR_TABLE[row[x]];
        if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

    }

    return 0;
}

int BPP_16(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 16 bits = RGB, the most significant bit is not used: (unused)_(5bitsRed)_(5bitsGreen)_(5bitsBlue)
    for (int x = 0; x < WIDTH; x++) {

        int R = row[x * 2] >> 2;
        int G = ((row[x * 2] << 6) >> 3) + (row[x * 2 + 1] >> 3);
        int B = (row[x * 2 + 1] << 3) >> 3;
        // well I could store 2 bytes in a short int then mask the bits using & and shift bits to the right with >>
        // but it was too late - I realized that after typing all those code

        if (fputc(CHARS[(int) round((double) ((R + G + B) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
    }

    return 0;
}

int BPP_24(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 3 btyes = BGR
    for (int x = 0; x < WIDTH; x++) {

        if (fputc(CHARS[(int) round((double) ((row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2]) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
    
    }

    return 0;
}

int BPP_32(int BPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 4 bytes = blue, green, red, and alpha
    // I'm treating transparency as a color component like R/ G/ B (it sounds like a good idea to me)
    for (int x = 0; x < WIDTH; x++) {
 
        if (fputc(CHARS[(int) round((double) ((row[x * 4] + row[x * 4 + 1] + row[x * 4 + 2]) / 1020.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

    }

    return 0;
}

int (*BPPs[6])(int, signed int, unsigned char*, PALETTE*, FILE*) = {BPP_1, BPP_4, BPP_8, BPP_16, BPP_24, BPP_32};

int BPP_TO_BPPs_INDEX(unsigned short int bpp) {
    switch (bpp) {
        case  1: return 1;
        case  4: return 2;
        case  8: return 3;
        case 16: return 4;
        case 24: return 5;
        case 32: return 6;
        default: return 0;
    }
}