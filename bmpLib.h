#ifndef BMP_LIB
#define BMP_LIB
#include <inttypes.h>

#define bPP_TO_bPPs_INDEX(bpp)         \
    _Generic(bpp,                      \
        uint16_t: bpp ==  1 ? 1 :      \
                  bpp ==  4 ? 2 :      \
                  bpp ==  8 ? 3 :      \
                  bpp == 16 ? 4 :      \
                  bpp == 24 ? 5 :      \
                  bpp == 32 ? 6 :      \
                              0 ,      \
        default: 0                     \
    )

typedef struct __attribute__((packed)) {
    uint16_t FILE_TYPE;
    uint32_t FILE_SIZE;
    uint16_t RESERVED0;
    uint16_t RESERVED1;
    uint32_t DATA_OFFSET;
} BMP_FILE_HEADER;

typedef struct __attribute__((packed)) {
    uint32_t FILE_HEADER_SIZE;
    int32_t  IMAGE_WIDTH;
    int32_t  IMAGE_HEIGHT;
    uint16_t COLOR_PLANE_COUNT;
    uint16_t BITS_PER_PIXEL;
    uint32_t COMPRESSION;
    uint32_t IMAGE_SIZE;
    int32_t  X_RESOLUTION_PPM; // PPM = Pixel Per Meter
    int32_t  Y_RESOLUTION_PPM;
    uint32_t USED_COLOR_COUNT;
    uint32_t IMPORTANT_COLOR_COUNT;
} BMP_INFO_HEADER;

typedef struct __attribute__((packed)) {
    uint8_t BLUE;
    uint8_t GREEN;
    uint8_t RED;
    uint8_t RESERVED;
} PALETTE;

enum COMPRESSION_METHODS { // global identifiers
  BI_RGB       = 0x0000,   // uncompressed
  BI_RLE8      = 0x0001,
  BI_RLE4      = 0x0002,
  BI_BITFIELDS = 0x0003,   // uncompressed, affects 16bpp (not RGB555 but RGB565) and 32bpp (unknown...)
  BI_JPEG      = 0x0004,
  BI_PNG       = 0x0005,
  BI_CMYK      = 0x000B,
  BI_CMYKRLE8  = 0x000C,
  BI_CMYKRLE4  = 0x000D
};

void handleError(const char* string, int nFile, int nAlloc, int total, ...);

int isSupported(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH);

int TO_ASCII(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH, PALETTE* COLOR_TABLE,
             FILE* BMP_FILE, FILE* TEXT_FILE,
             int (*f)(uint64_t, int32_t, uint8_t*, PALETTE*, FILE*, uint32_t));

int  bPP_1(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);
int  bPP_4(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);
int  bPP_8(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);
int bPP_16(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);
int bPP_24(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);
int bPP_32(uint64_t bPR, int32_t WIDTH, uint8_t* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE, uint32_t COMPRESSION);

extern int (*bPPs[6])(uint64_t, int32_t, uint8_t*, PALETTE*, FILE*, uint32_t);

#endif