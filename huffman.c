#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include "huffman.h"
#include "array.h"

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
        return 1;
    else if(*freq2 == NULL)
        return -1;

    if((*freq1)->count < (*freq2)->count)
        return 1;
    else if((*freq1)->count > (*freq2)->count)
        return -1;
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

frequency_t* huffman_build_tree(frequency_t* freqs, frequency_t* internalNodes)
{
    int listLen = 0;
    frequency_t** list = (frequency_t**) calloc(256, sizeof(frequency_t*));

    for(int i = 0; i < 256; i++)
    {
        list[i] = freqs + i;
    }

    qsort(list, 256, sizeof(frequency_t*), huffman_freq_sort);

    for(listLen = 0; listLen < 256 && list[listLen]->count > 0; listLen++);

    // FIXME FIXME FIXME: memory leak
    frequency_t* internalNodes = (frequency_t*) calloc(256, sizeof(frequency_t));
    int internalNodePos = 0;

    // FIXME: generalize these cases
    if(listLen == 1)
    {
        buildInternalNode(internalNodes + internalNodePos, list[0], list[1]);
        list[0] = internalNodes + internalNodePos;
    }

    while(listLen > 1)
    {
        qsort(list, listLen, sizeof(frequency_t*), huffman_freq_sort);
        buildInternalNode(internalNodes + internalNodePos, list[0], list[1]);
        list[0] = internalNodes + internalNodePos;
        list[1] = list[listLen - 1];
        list[listLen - 1] = NULL;
        listLen--;
        internalNodePos++;
    }

    frequency_t* retval = list[0];
    free(list);

    return retval;
}

array_t* huffman_encode_byte(frequency_t* root, char byte, array_t* acc)
{
    if(root->left == NULL || root->right == NULL)
    {
        if(root->val == byte)
        {
            return acc;
        }
        else
        {
            return NULL;
        }
    }

    // FIXME: shitloads of memory leaks
    unsigned char left_c = 0x00;
    array_t* left_acc = array_copy(acc);
    array_append(left_acc, &left_c);
    array_t* left_result = huffman_encode_byte(root->left, byte, left_acc);
    if(left_result != NULL)
    {
        return left_result;
    }
    free(left_acc);

    unsigned char right_c = 0x01;
    array_t* right_acc = array_copy(acc);
    array_append(right_acc, &right_c);
    array_t* right_result = huffman_encode_byte(root->right, byte, right_acc);
    if(right_result != NULL)
    {
        return right_result;
    }
    free(right_acc);

    // FIXME: don't think it can ever reach here, but I might be wrong
    return NULL;
}

array_t* huffman_encode(const unsigned char* data, int datalen)
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

    frequency_t* root = huffman_build_tree(freqs);      // FIXME: memory leak on this

    array_t* freq_array = array_create_from_pointer(freqs, 1, sizeof(frequency_t) * 256);   // use as char instead of frequency_t

    array_t* encoded_stream = array_create(1, 6);

    // for now, encode as chars; should be bitstream
    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        array_t* acc = array_create(1, 6);
        array_t* encoded_byte = huffman_encode_byte(root, *item, acc);

        encoded_stream = array_append_array(encoded_stream, encoded_byte);  // FIXME: leak

        // FIXME: not sure when this is true
        if(acc != encoded_byte)
        {
            array_free(acc);
        }

        array_free(encoded_byte);
    }

    array_t* outbitstream = array_append_array(freq_array, encoded_stream);

    //free(freqs);
    array_free(freq_array);
    array_free(encoded_stream);

    return outbitstream;
}

unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* dict = (frequency_t*) data;
    const unsigned char* indices = data + 256 * sizeof(frequency_t);

    frequency_t* root = huffman_build_tree(dict);

    array_t* out_array = array_create(1, 10000);

    int spos = 0;

    frequency_t* new_root = root;

    while(spos < datalen - 256 * sizeof(frequency_t))
    {
        if(new_root->left == NULL)
        {
            array_append(out_array, &new_root->val);
            new_root = root;
        }

        if(indices[spos] == 0x0)
        {
            new_root = new_root->left;
            spos++;
        }
        else if(indices[spos] == 0x1)
        {
            new_root = new_root->right;
            spos++;
        }
        else
        {
            assert(0);
        }
    }

    array_append(out_array, &new_root->val);

    // FIXME: abstraction
    unsigned char* outdata = (unsigned char*) realloc(out_array->base, out_array->len);
    *outdatalen = out_array->len;

    free(out_array);

    return outdata;
}
