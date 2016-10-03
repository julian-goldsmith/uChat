#ifndef DCT_H_INCLUDED
#define DCT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void dct_precompute_matrix();
void dct_encode_block(unsigned char yin[MB_SIZE][MB_SIZE], unsigned char uin[4][4],
                      unsigned char vin[4][4], short yout[MB_SIZE][MB_SIZE],
                      short uout[4][4], short vout[4][4]);
void dct_decode_block(const short yin[MB_SIZE][MB_SIZE], const short uin[4][4],
                      const short vin[4][4], unsigned char yout[MB_SIZE][MB_SIZE],
                      unsigned char uout[4][4], unsigned char vout[4][4]);

#ifdef __cplusplus
}
#endif

#endif // DCT_H_INCLUDED
