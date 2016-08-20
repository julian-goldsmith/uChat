#ifndef IMGCODER_H_INCLUDED
#define IMGCODER_H_INCLUDED

#define MB_SIZE 16
#define MB_NUM_X (640/MB_SIZE)
#define MB_NUM_Y (480/MB_SIZE)
#define NUMBLOCKS ((MB_NUM_X*MB_NUM_Y) / 6)  /*120*/

typedef struct
{
    unsigned char mb_x;
    unsigned char mb_y;
    double rms;
    unsigned char blockData[MB_SIZE][MB_SIZE][3];
    double blockDataDCT[3][MB_SIZE][MB_SIZE];
} macroblock_t;

void encodeImage(const unsigned char* imgIn, const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* rmsView);
void decodeImage(const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* frameOut);
void precomputeDCTMatrix();

#endif // IMGCODER_H_INCLUDED
