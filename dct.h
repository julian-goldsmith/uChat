#ifndef DCT_H_INCLUDED
#define DCT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void dct_precompute_matrix();
void dct_quantize_block(float data[MB_SIZE][MB_SIZE][4], short qdata[MB_SIZE][MB_SIZE][3]);
void dct_unquantize_block(short qdata[MB_SIZE][MB_SIZE][3], float data[MB_SIZE][MB_SIZE][4]);
void dct_encode_block(unsigned char blockData[MB_SIZE][MB_SIZE], float blockDataDCT[MB_SIZE][MB_SIZE][4]);
void dct_decode_block(float data[MB_SIZE][MB_SIZE][4], float pixels[MB_SIZE][MB_SIZE][4]);

#ifdef __cplusplus
}
#endif

#endif // DCT_H_INCLUDED
