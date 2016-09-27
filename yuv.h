#ifndef YUV_H_INCLUDED
#define YUV_H_INCLUDED

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char yout[MB_SIZE][MB_SIZE],
                unsigned char uout[MB_SIZE/4][MB_SIZE/4], unsigned char vout[MB_SIZE/4][MB_SIZE/4]);
void yuv_decode(const float yin[MB_SIZE][MB_SIZE][4], const unsigned char uin[MB_SIZE/4][MB_SIZE/4],
                const unsigned char vin[MB_SIZE/4][MB_SIZE/4], float out[MB_SIZE][MB_SIZE][4]);

#endif // YUV_H_INCLUDED
