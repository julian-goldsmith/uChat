#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include "huffman.h"

typedef struct
{
    unsigned char val;
    unsigned int count;
} frequency_t;

int huffman_freq_sort(const void* val1, const void* val2)
{
    const frequency_t* freq1 = (const frequency_t*) val1;
    const frequency_t* freq2 = (const frequency_t*) val2;

    if(freq1->count < freq2->count)
        return 1;
    else if(freq1->count > freq2->count)
        return -1;
    else
        return 0;
}

unsigned char* huffman_encode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* freqs = (frequency_t*) malloc(sizeof(frequency_t) * 256);

    for(unsigned int s = 0; s < 256; s++)
    {
        freqs[s].val = s;
    }

    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        freqs[*item].count++;
    }

    qsort(freqs, 256, sizeof(frequency_t), huffman_freq_sort);

    unsigned char* outdata = (unsigned char*) malloc(sizeof(unsigned char) * datalen + sizeof(frequency_t) * 256);
    int outdatapos = 0;

    // write dictionary
    memcpy(outdata + outdatapos, freqs, sizeof(frequency_t) * 256);
    outdatapos += sizeof(frequency_t) * 256;

    // for now, encode as shorts; should be bitstream
    // FIXME: there's probably a better way to do this loop
    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        for(unsigned int i = 0; i < 256; i++)
        {
            if(freqs[i].val == *item)
            {
                outdata[outdatapos++] = i;
                break;
            }
        }
    }

    free(freqs);

    *outdatalen = outdatapos;

    return outdata;
}

unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* dict = (frequency_t*) data;
    const unsigned char* indices = data + 256 * sizeof(frequency_t);

    unsigned char* outdata = (unsigned char*) malloc(sizeof(unsigned char) * datalen);
    int outdatapos = 0;

    for(const unsigned char* index = indices; index < data + datalen; index++)
    {
        outdata[outdatapos++] = dict[*index].val;
    }

    outdata = (unsigned char*) realloc(outdata, outdatapos);

    *outdatalen = outdatapos;

    return outdata;
}
