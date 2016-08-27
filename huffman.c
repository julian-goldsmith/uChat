#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include "huffman.h"
#include "array.h"

typedef struct frequency_s
{
    unsigned int count;
    bool is_leaf;
    struct frequency_s* left;
    struct frequency_s* right;
    unsigned char val;
} frequency_t;

int huffman_freq_sort(const void* val1, const void* val2)
{
    const frequency_t** freq1 = (const frequency_t**) val1;
    const frequency_t** freq2 = (const frequency_t**) val2;

    // FIXME: can probably get rid of NULL checks
    if(*freq1 == NULL && *freq2 == NULL)
        return 0;
    else if(*freq1 == NULL)
        return 1;
    else if(*freq2 == NULL)
        return -1;

    if((*freq1)->count < (*freq2)->count)
        return -1;
    else if((*freq1)->count > (*freq2)->count)
        return 1;
    else
        return 0;
}

frequency_t* buildInternalNode(frequency_t* left, frequency_t* right)
{
    frequency_t* node = (frequency_t*) malloc(sizeof(frequency_t));
    node->count = left->count + right->count;
    node->is_leaf = false;
    node->left = left;
    node->right = right;
    node->val = 0;
    return node;
}

void freeTree(frequency_t* root)
{
    if(root->left != NULL)
    {
        freeTree(root->left);
    }

    if(root->right != NULL)
    {
        freeTree(root->right);
    }

    free(root);
}

frequency_t* huffman_build_tree(frequency_t** freqs)
{
    int listLen = 256;
    frequency_t** list = (frequency_t**) calloc(256, sizeof(frequency_t*));

    for(int i = 0; i < 256; i++)
    {
        list[i] = freqs[i];
    }

    while(listLen > 1)
    {
        qsort(list, listLen, sizeof(frequency_t*), huffman_freq_sort);
        list[0] = buildInternalNode(list[0], list[1]);
        list[1] = list[listLen - 1];
        list[listLen - 1] = NULL;       // FIXME: can probably get rid of
        listLen--;
    }

    frequency_t* retval = list[0];
    free(list);

    return retval;
}

bool huffman_encode_byte(frequency_t* root, unsigned char byte, array_t* acc)
{
    if(root == NULL)
    {
        return false;
    }

    if(root->val == byte && root->is_leaf)
    {
        return true;
    }

    unsigned char right_c = 0x1;
    array_append(acc, &right_c);

    bool right_result = huffman_encode_byte(root->right, byte, acc);
    if(right_result)
    {
        return right_result;
    }

    array_pop(acc);

    unsigned char left_c = 0x0;
    array_append(acc, &left_c);

    bool left_result = huffman_encode_byte(root->left, byte, acc);
    if(left_result)
    {
        return left_result;
    }

    array_pop(acc);

    return false;
}

array_t* huffman_encode(const unsigned char* data, int datalen)
{
    frequency_t** freqs = (frequency_t**) malloc(sizeof(frequency_t*) * 256);

    for(unsigned int s = 0; s < 256; s++)
    {
        freqs[s] = (frequency_t*) malloc(sizeof(frequency_t));
        freqs[s]->val = s;
        freqs[s]->count = 0;
        freqs[s]->is_leaf = true;
        freqs[s]->left = NULL;
        freqs[s]->right = NULL;
    }

    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        freqs[*item]->count++;
    }

    frequency_t* root = huffman_build_tree(freqs);      // FIXME: memory leak on this

    array_t* freq_array = array_create(sizeof(frequency_t), 256);

    for(int i = 0; i < 256; i++)
    {
        array_append(freq_array, freqs[i]);
    }

    array_t* encoded_stream = array_create(1, 6);

    // for now, encode as chars; should be bitstream
    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        bool enc = huffman_encode_byte(root, *item, encoded_stream);

        if(enc == false)
        {
            assert(enc);
        }
    }

    free(freqs);
    freeTree(root);

    // FIXME: hack
    freq_array->len *= freq_array->item_size;
    freq_array->capacity *= freq_array->item_size;
    freq_array->item_size = 1;

    array_append_array(freq_array, encoded_stream);

    array_free(encoded_stream);

    return freq_array;
}

unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* dict = (frequency_t*) data;
    const unsigned char* indices = data + 256 * sizeof(frequency_t);

    frequency_t** freqs = (frequency_t**) malloc(sizeof(frequency_t*) * 256);

    for(int i = 0; i < 256; i++)
    {
        freqs[i] = (frequency_t*) malloc(sizeof(frequency_t));
        freqs[i]->val = dict[i].val;
        freqs[i]->count = dict[i].count;
        freqs[i]->is_leaf = true;
        freqs[i]->left = NULL;
        freqs[i]->right = NULL;
    }

    frequency_t* root = huffman_build_tree(freqs);

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
    freeTree(root);

    return outdata;
}
