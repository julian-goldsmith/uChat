#ifndef HUFFMAN_H_INCLUDED
#define HUFFMAN_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* huffman_encode(const unsigned char* data, int datalen, int* outdatalen);
unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen);

#ifdef __cplusplus
}
#endif


#endif // HUFFMAN_H_INCLUDED
