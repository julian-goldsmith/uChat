#ifndef BWT_H_INCLUDED
#define BWT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

short* bwt_encode(const short* inarr, unsigned short* posp);
short* bwt_decode(short outs[16], unsigned short pos);

#ifdef __cplusplus
}
#endif

#endif // BWT_H_INCLUDED
