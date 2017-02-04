#ifndef IMGCODER_H_INCLUDED
#define IMGCODER_H_INCLUDED

#define MB_SIZE 16
#define MB_NUM_X (640/MB_SIZE)
#define MB_NUM_Y (480/MB_SIZE)

typedef struct
{
    unsigned char mb_x;
    unsigned char mb_y;
    float rms;
    unsigned char blockData[MB_SIZE][MB_SIZE][3];
} macroblock_t;

typedef struct
{
    short magic;
    unsigned char mb_x;
    unsigned char mb_y;
    short yout[MB_SIZE][MB_SIZE];
    short uout[4][4];
    short vout[4][4];
} compressed_macroblock_t;

#ifdef __cplusplus
extern "C" {
#endif

compressed_macroblock_t* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame, unsigned char* rmsView, short* num_blocks);
void ic_decode_image(const unsigned char* prevFrame, const compressed_macroblock_t* cblocks, const short numBlocks, unsigned char* frameOut);
void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks, short numBlocks);

#ifdef __cplusplus
}
#endif

#endif // IMGCODER_H_INCLUDED
