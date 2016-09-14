#ifndef YUV_H_INCLUDED
#define YUV_H_INCLUDED

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char out[MB_SIZE][MB_SIZE][3]);
void yuv_decode(float in[MB_SIZE][MB_SIZE][4], float out[MB_SIZE][MB_SIZE][4]);

#endif // YUV_H_INCLUDED
