#ifndef LZW_H_INCLUDED
#define LZW_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

array_t* lz_encode(unsigned char* file_data, int file_len);
array_t* lz_decode(array_t* enc_data);

#ifdef __cplusplus
}
#endif

#endif // LZW_H_INCLUDED
