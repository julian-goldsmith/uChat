#ifndef YUV_H_INCLUDED
#define YUV_H_INCLUDED

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char yout[MB_SIZE][MB_SIZE],
                unsigned char uout[4][4], unsigned char vout[4][4]);
void yuv_decode(const unsigned char yin[MB_SIZE][MB_SIZE], const unsigned char uin[4][4],
                const unsigned char vin[4][4], unsigned char out[MB_SIZE][MB_SIZE][3]);

#endif // YUV_H_INCLUDED
