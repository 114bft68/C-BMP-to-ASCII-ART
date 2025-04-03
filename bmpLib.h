#ifndef BMP_LIB
#define BMP_LIB

#define bPP_TO_bPPs_INDEX(bpp) _Generic(bpp, \
    unsigned short int: bpp ==  1 ? 1 :      \
                        bpp ==  4 ? 2 :      \
                        bpp ==  8 ? 3 :      \
                        bpp == 16 ? 4 :      \
                        bpp == 24 ? 5 :      \
                        bpp == 32 ? 6 :      \
                                    0 ,      \
    default: 0                               \
)

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

typedef struct __attribute__((packed)) {
    unsigned char BLUE;
    unsigned char GREEN;
    unsigned char RED;
    unsigned char RESERVED;
} PALETTE;

extern int (*bPPs[6])(int, signed int, unsigned char*, PALETTE*, FILE*);

void handleError(const char* string, int nFile, int nAlloc, int total, ...);

int TO_ASCII(BMP_FILE_HEADER* BFH, BMP_INFO_HEADER* BIH, PALETTE* COLOR_TABLE,
             FILE* BMP_FILE, FILE* TEXT_FILE,
             int (*f)(int, signed int, unsigned char*, PALETTE*, FILE*));

int  bPP_1(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);
int  bPP_4(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);
int  bPP_8(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);
int bPP_16(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);
int bPP_24(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);
int bPP_32(int bPR, signed int WIDTH, unsigned char* row, PALETTE* COLOR_TABLE, FILE* TEXT_FILE);

#endif