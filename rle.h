#ifndef RLE_H_INCLUDED
#define RLE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

float* rle_encode_block(float data[MB_SIZE][MB_SIZE][4], int* rleSize);
void rle_decode_block(float data[MB_SIZE][MB_SIZE][4], float* rleData, int rleSize);

#ifdef __cplusplus
}
#endif

#endif // RLE_H_INCLUDED
