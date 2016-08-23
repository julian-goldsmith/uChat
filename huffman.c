#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include "huffman.h"

typedef struct frequency_s
{
    unsigned int count;
    struct frequency_s* left;
    struct frequency_s* right;
    unsigned char val;
} frequency_t;

int huffman_freq_sort(const void* val1, const void* val2)
{
    const frequency_t** freq1 = (const frequency_t**) val1;
    const frequency_t** freq2 = (const frequency_t**) val2;

    if(*freq1 == NULL && *freq2 == NULL)
        return 0;
    else if(*freq1 == NULL)
        return -1;
    else if(*freq2 == NULL)
        return 1;

    if(*freq1->count < *freq2->count)
        return -1;
    else if(*freq1->count > *freq2->count)
        return 1;
    else
        return 0;
}

void buildInternalNode(frequency_t* node, frequency_t* left, frequency_t* right)
{
    node->count = left->count + right->count;
    node->left = left;
    node->right = right;
    node->val = 0;
}

frequency_t* huffman_build_tree(frequency_t* freqs)
{
    int listLen = 0;
    frequency_t** list = (frequency_t**) calloc(1024, sizeof(frequency_t*));

    for(int i = 0; i < 256; i++)
    {
        list[i] = freqs[i];
        listLen++;
    }

    for(int i = 256; i < 1024; i++)
    {
        list[i] = NULL;
    }

    // FIXME FIXME FIXME: memory leak
    frequency_t* internalnodes = (frequency_t*) calloc(1024, sizeof(frequency_t));
    int internalNodePos = 0;

    while(listLen > 1)
    {
        qsort(list, listLen, sizeof(frequency_t*), huffman_freq_sort);
        buildInternalNode(internalNodes + internalNodePos, list[0], list[1]);
        list[0] = internalNodes + internalNodePos;
        list[1] = NULL;
        listLen--;
        internalNodePos++;
    }

    internalNodes = (frequency_t*) realloc(internalnodes, sizeof(frequency_t*) * internalNodePos);

    frequency_t* retval = list[0];
    free(list);

    return retval;
}

unsigned char* huffman_encode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* freqs = (frequency_t*) malloc(sizeof(frequency_t) * 256);

    for(unsigned int s = 0; s < 256; s++)
    {
        freqs[s].val = s;
        freqs[s].count = 0;
        freqs[s].left = NULL;
        freqs[s].right = NULL;
    }

    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        freqs[*item].count++;
    }

    frequency_t* root = huffman_build_tree(freqs);

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
