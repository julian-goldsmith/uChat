#ifndef BWT_H_INCLUDED
#define BWT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void bwt_encode(const short* inarr, short outarr[256], short indexlist[256]);
void bwt_decode(short outs[256], short indexlist[256], short outv[256]);

#ifdef __cplusplus
}
#endif

#endif // BWT_H_INCLUDED
