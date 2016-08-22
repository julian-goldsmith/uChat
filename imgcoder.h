#ifndef IMGCODER_H_INCLUDED
#define IMGCODER_H_INCLUDED

#define MB_SIZE 16
#define MB_NUM_X (640/MB_SIZE)
#define MB_NUM_Y (480/MB_SIZE)
//#define NUMBLOCKS ((MB_NUM_X*MB_NUM_Y) / 16)  /*120*/

typedef struct
{
    unsigned char mb_x;
    unsigned char mb_y;
    float rms;
    unsigned char blockData[MB_SIZE][MB_SIZE][3];
} macroblock_t;

typedef struct
{
    unsigned char mb_x;
    unsigned char mb_y;
    int rleSize;
    short* rleData;
} compressed_macroblock_t;

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame, unsigned char* rmsView, int* totalSize);
void ic_decode_image(const unsigned char* prevFrame, const unsigned char* data, unsigned char* frameOut);

#ifdef __cplusplus
}
#endif

#endif // IMGCODER_H_INCLUDED
