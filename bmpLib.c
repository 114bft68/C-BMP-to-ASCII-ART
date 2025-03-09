#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "bmpLib.h"

const char CHARS[] = " .-_!()^*%&#$@";
// const char CHARS[] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
// ^^^^^^^^^^^^^^^^^^ had to comment this out because the spell check function in text editors messed up
#define CHARS_LEN strlen(CHARS)
#define min(x, y) ({ \
    typeof(x) _x = x; \
    typeof(y) _y = y; \
    _x > _y ? _x : _y; \
})

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

    int bPR_NO_PADDING = BIH->BITS_PER_PIXEL * BIH->IMAGE_WIDTH; // bits per row without padding
    
    if (bPR_NO_PADDING < 8) {
        printf("\nThe BMP image is too small\n");
        return 1;
        // this is way too small (not impossible to turn it into an ASCII art but it requires some efforts...)
        // you need to determine if bPR_NO_PADDING is lower than 8 then implement the loop inside bPP_(1, 4, 8, 16, 24, 32) so that the step is 1 bit
        // but this type of images is not common and wouldn't generate a good-looking ASCII art
    }

    int bytesPerRow  = (bPR_NO_PADDING + (bPR_NO_PADDING % 32 == 0 ? 0 : (32 - bPR_NO_PADDING % 32))) / 8; // (bits per row + padding) / 8

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

        if (f(bPR_NO_PADDING, BIH->IMAGE_WIDTH, row, COLOR_TABLE, TEXT_FILE)) {
            handleError("\nUnable to write into the text file\n", 0, 1, 1, (void*) row);
            return 1;
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
            return 1;
        }
    }

    free(row); // freeing memeory before exiting
    return 0;
}

int bPP_1(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 bit = 1 PALETTE = 1 pixel
    // bits per row without padding / 8 (truncated if BPR / 8 is a float)

    int bytesPerRow   = (int) (bPR / 8); // truncated, not including bPR % 8 (the remaining bits)
    int remainingBits = min(bPR % 8, 1); // the remaining bits (if they exist, this must be 1, else 0)

    for (int x = 0; x < bytesPerRow + remainingBits; x++) {

        for (int i = 7; i >= 0; i--) {

            PALETTE BGR = COLOR_TABLE[(row[x] & (1 << i)) > 0 ? 1 : 0];
            if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

            if (remainingBits && x == bytesPerRow && bPR % 8 + i == 8) break; // original: 7 - i == bPR % 8 - 1
            // if remainingBits = 0 then the following statements won't be evaluated (improves the performance negligibly)
            
        }
    }

    /* previous function (it might be better or worse idk):

    for (int x = 0; x < (int) (bPR / 8); x++) {

        for (int i = 7; i >= 0; i--) {
            
            PALETTE BGR = COLOR_TABLE[row[x] & (1 << i) > 0 ? 1 : 0];
            if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

        }
    }

    int remainder = bPR % 8;
    if (remainder) {

        for (int i = 7; i >= remainder; i--) {

            PALETTE BGR = COLOR_TABLE[row[(((int) (bPR / 8)) & (1 << i)) > 0 ? 1 : 0]];
            if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
        }
    }
    
    */

    return 0;
}

int bPP_4(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // each of the first 4 bits and the last 4 bits represent a PALETTE which = a pixel

    int bytesPerRow   = (int) (bPR / 8); // not including the remaining bits
    int remainingBits = min(bPR % 8, 1); // min((either 4 or 0), 1) => either 1 or 0

    for (int x = 0; x < bytesPerRow + remainingBits; x++) {

        PALETTE entries[] = {
            COLOR_TABLE[row[x] >> 4],
            COLOR_TABLE[row[x] & 0b00001111]
        };

        if (fputc(CHARS[(int) round((double) ((entries[0].BLUE + entries[0].GREEN + entries[0].RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) 
            return 1;

        if (x == bytesPerRow) break;
        // if remainingBits = 1 then the last loop would be bytesPerRow and there should only be 4 remaining bits to read, so the next 4 bits doesn't represent any color

        if (fputc(CHARS[(int) round((double) ((entries[1].BLUE + entries[1].GREEN + entries[1].RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) 
            return 1;
    }

    /* previous function (it might be better or worse)

    for (int x = 0; x < (int) (bPR / 8); x++) {

        PALETTE entries[] = {
            COLOR_TABLE[row[x] >> 4],
            COLOR_TABLE[row[x] & 0b00001111]
        };
    
        for (int i = 0; i < 2; i++) {
            if (fputc(CHARS[(int) round((double) ((entries[i].BLUE + entries[i].GREEN + entries[i].RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) 
                return 1;
        }

    }

    int remainder = bPR % 8;
    if (remainder) { // 4 bits for 1 pixel

        PALETTE BGR = COLOR_TABLE[row[((int) (bPR / 8)) >> 4]];
        if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
    }

    */
    
    return 0;
}

int bPP_8(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 byte = 1 PALETTE = 1 pixel
    for (int x = 0; x < WIDTH; x++) {
    
        PALETTE BGR = COLOR_TABLE[row[x]];
        if (fputc(CHARS[(int) round((double) ((BGR.BLUE + BGR.GREEN + BGR.RED) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

    }

    return 0;
}

int bPP_16(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 16 bits = RGB = 1 pixel, the most significant bit is not used: (unused)_(5bitsRed)_(5bitsGreen)_(5bitsBlue)
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

int bPP_24(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 3 btyes = BGR = a pixel
    for (int x = 0; x < WIDTH; x++) {

        if (fputc(CHARS[(int) round((double) ((row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2]) / 765.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;
    
    }

    return 0;
}

int bPP_32(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 4 bytes = blue, green, red, and alpha = a pixel
    // I'm treating transparency as a color component like R/ G/ B (it sounds like a good idea to me)
    for (int x = 0; x < WIDTH; x++) {
 
        if (fputc(CHARS[(int) round((double) ((row[x * 4] + row[x * 4 + 1] + row[x * 4 + 2]) / 1020.0 * (CHARS_LEN - 1)))], TEXT_FILE) == EOF) return 1;

    }

    return 0;
}

int (*bPPs[6])(int, signed int, unsigned char*, PALETTE*, FILE*) = {bPP_1, bPP_4, bPP_8, bPP_16, bPP_24, bPP_32};

int bPP_TO_bPPs_INDEX(unsigned short int bpp) {
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