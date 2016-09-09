#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include "huffman.h"
#include "bitstream.h"

int huffman_freq_sort(const void* val1, const void* val2)
{
    const node_t** freq1 = (const node_t**) val1;
    const node_t** freq2 = (const node_t**) val2;

    if((*freq1)->count < (*freq2)->count)
        return -1;
    else if((*freq1)->count > (*freq2)->count)
        return 1;
    else
        return 0;
}

node_t* buildInternalNode(node_t* left, node_t* right, array_t* all_nodes)
{
    node_t* node = (node_t*) array_get_new(all_nodes);
    node->count = left->count + right->count;
    node->is_leaf = false;
    node->left = left;
    node->right = right;
    node->val = 0;

    return node;
}

node_t* huffman_build_tree(frequency_t freqs[256], array_t* all_nodes)
{
    int listLen = 256;
    node_t* list[256];

    for(int i = 0; i < 256; i++)
    {
        list[i] = (node_t*) array_get_new(all_nodes);
        list[i]->val = freqs[i].val;
        list[i]->count = freqs[i].count;
        list[i]->is_leaf = true;
        list[i]->left = NULL;
        list[i]->right = NULL;
    }

    while(listLen > 1)
    {
        qsort(list, listLen, sizeof(node_t*), huffman_freq_sort);
        list[0] = buildInternalNode(list[0], list[1], all_nodes);
        list[1] = list[listLen - 1];
        listLen--;
    }

    node_t* retval = list[0];

    return retval;
}

// keeps track of where we are
typedef enum {ST_RIGHT, ST_LEFT, ST_DONE} statew_t;

typedef struct
{
    statew_t state;
    node_t* node;
} state_t;

void huffman_encode_byte(node_t* root, unsigned char byte, int count, array_t* history)
{
    state_t* initial_state = (state_t*) array_get_new(history);
    initial_state->state = ST_RIGHT;
    initial_state->node = root;

    while(true)
    {
        // put our state in a local variable, so it's not overwritten
        state_t* curr_state = array_get(history, history->len - 1);

        if(curr_state->state == ST_RIGHT || curr_state->state == ST_LEFT)
        {
            node_t* next_node = (curr_state->state == ST_RIGHT) ? curr_state->node->right : curr_state->node->left;

            // update current state
            curr_state->state = curr_state->state + 1;

            // if we have the value we want, bail out; if we have a non-leaf node, save;
            //     otherwise, continue and don't save next state
            if(next_node->is_leaf && next_node->val == byte)
            {
                return;
            }
            else if(!next_node->is_leaf && next_node->count >= count)
            {
                state_t* next_state = (state_t*) array_get_new(history);
                next_state->state = ST_RIGHT;
                next_state->node = next_node;
            }
        }
        else
        {
            array_pop(history);
        }
    }
}

void huffman_flatten_history(array_t* history, bitstream_t* bs)
{
    for(int i = 0; i < history->len; i++)
    {
        state_t* state = (state_t*) array_get(history, i);
        bitstream_append(bs, state->state == ST_LEFT);
    }
}

array_t* huffman_encode(const unsigned char* data, int datalen, unsigned short frequencies[256], unsigned int* bit_len)
{
    frequency_t freqs[256];
    array_t* all_nodes = array_create(sizeof(node_t), 512);

    for(unsigned int s = 0; s < 256; s++)
    {
        freqs[s].val = s;
        freqs[s].count = 0;
    }

    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        freqs[*item].count++;
    }

    node_t* root = huffman_build_tree(freqs, all_nodes);

    bitstream_t* encoded_stream = bitstream_create();
    array_t* history = array_create(sizeof(state_t), 8);

    for(const unsigned char* item = data; item < data + datalen; item++)
    {
        array_clear(history);

        huffman_encode_byte(root, *item, freqs[*item].count, history);
        huffman_flatten_history(history, encoded_stream);
    }

    array_free(history);
    array_free(all_nodes);

    bitstream_array_adjust(encoded_stream);

    array_t* out_array = array_create(1, encoded_stream->array->len);
    array_append_array(out_array, encoded_stream->array);

    for(int i = 0; i < 256; i++)
    {
        frequencies[i] = freqs[i].count;
    }

    *bit_len = encoded_stream->pos;

    bitstream_free(encoded_stream);

    return out_array;
}

array_t* huffman_decode(const unsigned char* data, int datalen, const unsigned short frequencies[256], unsigned int bit_len)
{
    frequency_t freqs[256];
    array_t* all_nodes = array_create(sizeof(node_t), 512);

    for(int i = 0; i < 256; i++)
    {
        freqs[i].val = i;
        freqs[i].count = frequencies[i];
    }

    node_t* root = huffman_build_tree(freqs, all_nodes);

    int spos = 0;

    node_t* new_root = root;

    array_t* out_array = array_create(1, bit_len / 8 + 1);

    bitstream_t* bs = (bitstream_t*) malloc(sizeof(bitstream_t));
    bs->array = array_create_from_pointer(data, 1, datalen);
    bs->pos = 0;

    while(spos < bit_len)
    {
        if(bitstream_read(bs))
        {
            new_root = new_root->right;
        }
        else
        {
            new_root = new_root->left;
        }

        if(new_root->is_leaf)
        {
            array_append(out_array, &new_root->val);
            new_root = root;
        }

        spos++;
    }

    array_free(all_nodes);

    return out_array;
}
