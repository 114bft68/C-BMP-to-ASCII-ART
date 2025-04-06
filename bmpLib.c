#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include "bmpLib.h"

const char CHARS[] = " .-_!()^*%&#$@";     // dark mode  --> space is the darkest
// const char CHARS[] = "@$#&%*^)(!_-. ";  // light mode --> space is the brightest
// const char CHARS[] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
#define CHARS_LEN strlen(CHARS)
#define SELECT_CHAR(sum) CHARS[((int) round((double) ((sum) / 765.0 * (CHARS_LEN - 1))))]

#define min(x, y) ({   \
    typeof(x) _x = x;  \
    typeof(y) _y = y;  \
    _x > _y ? _x : _y; \
})

int (*bPPs[6])(uint64_t, int32_t, uint8_t*, PALETTE*, FILE*) = {bPP_1, bPP_4, bPP_8, bPP_16, bPP_24, bPP_32};

void handleError(const char* string, int nFile, int nAlloc, int total, ...) {
/*****************************************************************************************************************\
* + string: the message to be printed                                                                             *
* + nFile : the count of files to be closed                                                                       *
* + nAlloc: the count of allocated memory to free                                                                 *
* + total : nFile + nAlloc (the second parameter of va_start() must be the last fixed parameter of this function) *
\*****************************************************************************************************************/
    va_list items;
    va_start(items, total);
    
    for (int i = 0; i < nFile; ++i) {
        FILE* file = va_arg(items, FILE*);
        if (file) {
            fclose(file);
            file = NULL;
        }
    }
    
    for (int i = nFile; i < nAlloc; ++i) {
        void* ptr = va_arg(items, void*);
        if (ptr) {
            free(ptr);
            ptr = NULL;
        }
    }
    
    va_end(items);
    
    printf("%s", string);
}

int isSupported(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH) {
    return
        BFH->FILE_TYPE == 0x4D42               && // is BMP
        BIH->IMAGE_HEIGHT != 0                 && // height is not 0
        BIH->IMAGE_WIDTH > 0                   && // width <= 0 (a negative height flips the image vertically but I'm not sure if a negative width flips the image data horizontally)
        bPP_TO_bPPs_INDEX(BIH->BITS_PER_PIXEL) && // if >0 value is returned, the BMP is supported (bPP is either 1, 4, 8, 16, 24, or 32)
        !BIH->COMPRESSION                       ; // if the BMP is not compressed, !0 (true) is returned (compressed BMPs are not supported)
}

int TO_ASCII(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH, PALETTE* COLOR_TABLE,
             FILE* BMP_FILE, FILE* TEXT_FILE,
             int (*f)(uint64_t, int32_t, uint8_t*, PALETTE*, FILE*)) {

    uint64_t bPR_NO_PADDING = BIH->BITS_PER_PIXEL * BIH->IMAGE_WIDTH;
    // bits per row without padding
    // width must be >0 (isSupported function)
    // bPR_NO_PADDING is actually only around 47 bits maximally
    
    if (bPR_NO_PADDING < 8) {
        puts("\nThe BMP image is too small");
        return 1;
        // this is way too small (not impossible to turn it into an ASCII art but it requires some efforts...)
        // you need to determine if bPR_NO_PADDING is lower than 8 then implement the loop inside bPP_(1, 4, 8, 16, 24, 32) so that the step is 1 bit
        // but this type of images is not common and wouldn't generate a good-looking ASCII art
    }

    uint64_t bytesPerRow  = (bPR_NO_PADDING + (bPR_NO_PADDING % 32 == 0 ? 0 : (32 - bPR_NO_PADDING % 32))) / 8; // (bits per row + padding) / 8

    if (fseek(BMP_FILE, BFH->DATA_OFFSET + (BIH->IMAGE_HEIGHT < 0 ? 0 : bytesPerRow * (BIH->IMAGE_HEIGHT - 1)), SEEK_SET)) {
        perror("\nUnable to move file pointer");
        return 1;
    }

    uint8_t* row = malloc(bytesPerRow);
    if (!row) {
        puts("\nUnable to allocate memory for reading");
        return 1;
    }

    for (uint32_t y = 0; y < abs(BIH->IMAGE_HEIGHT); ++y) {
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

int bPP_1(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 bit = 1 PALETTE = 1 pixel
    // bits per row without padding / 8 (truncated if BPR / 8 is a float)
    // traded the aesthetic off for performance
    // (the original code achieved the same result with less lines of code but there was an extra if statement being evaluated every iteration)

    uint64_t bytesPerRow = (uint64_t) (bPR / 8);  // truncated, not including the remaining bits
    int remainingBits = bPR % 8;

    for (uint64_t x = 0; x < bytesPerRow; ++x) {

        for (int i = 7; i >= 0; --i) {

            PALETTE BGR = COLOR_TABLE[(row[x] & (1 << i)) > 0 ? 1 : 0];
            if (fputc(SELECT_CHAR(BGR.BLUE + BGR.GREEN + BGR.RED), TEXT_FILE) == EOF) return 1;

        }

    }

    if (remainingBits) {
        // handling the remaining bit(s) (if they exist)

        for (int i = remainingBits; i >= 0; --i) {

            PALETTE BGR = COLOR_TABLE[(row[bytesPerRow] & (1 << i)) > 0 ? 1 : 0];
            if (fputc(SELECT_CHAR(BGR.BLUE + BGR.GREEN + BGR.RED), TEXT_FILE) == EOF) return 1;

        }

    }

    return 0;
}

int bPP_4(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // each of the first 4 bits and the last 4 bits represent a PALETTE which = a pixel
    // traded the aesthetic off for performance
    // (the original code achieved the same result with less lines of code but there was an extra if statement being evaluated every iteration)

    uint64_t bytesPerRow = (uint64_t) (bPR / 8);

    for (uint64_t x = 0; x < bytesPerRow; ++x) {

        PALETTE entries[] = {
            COLOR_TABLE[row[x] >> 4],
            COLOR_TABLE[row[x] & 15] // 15 = 0b00001111
        };

        for (int i = 0; i < 2; ++i) {

            if (fputc(SELECT_CHAR(entries[i].BLUE + entries[i].GREEN + entries[i].RED), TEXT_FILE) == EOF)
                return 1;
        
        }

    }

    if (bPR % 8) {
        // handling the remaining bit(s), the remaining bit(s) should be either 4 or 0

        PALETTE entry = COLOR_TABLE[row[bytesPerRow] >> 4];

        if (fputc(SELECT_CHAR(entry.BLUE + entry.GREEN + entry.RED), TEXT_FILE) == EOF)
            return 1;
    
    }
    
    return 0;
}

int bPP_8(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 1 byte = 1 PALETTE = 1 pixel
    for (int32_t x = 0; x < WIDTH; ++x) {
    
        PALETTE BGR = COLOR_TABLE[row[x]];
        if (fputc(SELECT_CHAR(BGR.BLUE + BGR.GREEN + BGR.RED), TEXT_FILE) == EOF) return 1;

    }

    return 0;
}

int bPP_16(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 16 bits = RGB = 1 pixel, the most significant bit is not used: (unused)_(5bitsRed)_(5bitsGreen)_(5bitsBlue)
    for (int32_t x = 0; x < WIDTH; ++x) {

        if (fputc(SELECT_CHAR(
                             (row[x * 2] >> 2)                                  + // red
                             (((row[x * 2] << 6) >> 3) + (row[x * 2 + 1] >> 3)) + // green
                             ((row[x * 2 + 1] << 3) >> 3)                         // blue
                  ), TEXT_FILE) == EOF) return 1;

    } // messy

    return 0;
}

int bPP_24(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 3 btyes = BGR = a pixel
    for (int32_t x = 0; x < WIDTH; ++x) {

        if (fputc(SELECT_CHAR(row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2]), TEXT_FILE) == EOF) return 1;
    
    }

    return 0;
}

int bPP_32(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE) {
    // 4 bytes = blue, green, red, and alpha = a pixel
    // I'm treating transparency as a color component like R/ G/ B (it sounds like a good idea to me)
    for (int32_t x = 0; x < WIDTH; ++x) {
 
        if (fputc(SELECT_CHAR(row[x * 4] + row[x * 4 + 1] + row[x * 4 + 2] + row[x * 4 + 3]), TEXT_FILE) == EOF) return 1;

    }

    return 0;
}