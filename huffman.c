#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include "huffman.h"
#include "bitstream.h"

typedef struct node_s
{
    unsigned int count;
    bool is_leaf;
    struct node_s* left;
    struct node_s* right;
    struct node_s* parent;
    unsigned char val;
} node_t;

typedef struct
{
    unsigned int count;
    unsigned char val;
} frequency_t;

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
    node->parent = NULL;
    node->val = 0;

    node->left->parent = node;
    node->right->parent = node;

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
        list[i]->parent = NULL;
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

bool huffman_encode_byte(node_t* root, unsigned char byte, bitstream_t* acc, array_t* history)
{
    state_t* temp_state = (state_t*) array_get_new(history);
    temp_state->state = ST_RIGHT;
    temp_state->node = root;

    bitstream_append(acc, true);

    while(true)
    {
        temp_state = array_get(history, history->len - 1);
        state_t curr_state = { .state = temp_state->state, .node = temp_state->node };
        array_pop(history);

        if(curr_state.state == ST_RIGHT || curr_state.state == ST_LEFT)
        {
            bool val = bitstream_peek(acc);

            if(curr_state.state == ST_LEFT || curr_state.node == root)
            {
                bitstream_pop(acc);
            }

            if(curr_state.node != root && !(val == (curr_state.state == ST_RIGHT)))
            {
                val = true;
            }

            state_t* updated_state = (state_t*) array_get_new(history);
            updated_state->state = curr_state.state + 1;
            updated_state->node = curr_state.node;

            node_t* tnode = (curr_state.state == ST_RIGHT) ? curr_state.node->right : curr_state.node->left;

            bitstream_append(acc, curr_state.state == ST_RIGHT);

            if(tnode->is_leaf)
            {
                if(tnode->val == byte)
                {
                    return true;
                }
            }
            else
            {
                state_t* new_state = (state_t*) array_get_new(history);
                new_state->state = ST_RIGHT;
                new_state->node = tnode;
            }
        }
        else
        {
            bitstream_pop(acc);
        }
    }
}

array_t* huffman_encode(const unsigned char* data, int datalen)
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

        bool enc = huffman_encode_byte(root, *item, encoded_stream, history);

        if(enc == false)
        {
            assert(enc);
        }
    }

    array_free(history);
    array_free(all_nodes);

    bitstream_array_adjust(encoded_stream);

    // FIXME: hack-ish
    array_t* freq_array = array_create_from_pointer(&freqs, 1, sizeof(frequency_t) * 256);
    array_t* length_array = array_create_from_pointer(&encoded_stream->pos, 1, 4);

    array_t* out_array = array_create(1, freq_array->len + encoded_stream->array->len + length_array->len);
    array_append_array(out_array, freq_array);
    array_append_array(out_array, length_array);
    array_append_array(out_array, encoded_stream->array);

    bitstream_free(encoded_stream);

    return out_array;
}

unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen)
{
    frequency_t* dict = (node_t*) data;
    const unsigned char* indices = data + 256 * sizeof(frequency_t);

    frequency_t freqs[256];
    array_t* all_nodes = array_create(sizeof(node_t), 512);

    for(int i = 0; i < 256; i++)
    {
        freqs[i].val = dict[i].val;
        freqs[i].count = dict[i].count;
    }

    node_t* root = huffman_build_tree(freqs, all_nodes);

    int spos = 0;

    int bit_len = *(unsigned int*) indices;
    indices += 4;

    node_t* new_root = root;

    array_t* out_array = array_create(1, bit_len / 8 + 1);

    bitstream_t* bs = (bitstream_t*) malloc(sizeof(bitstream_t));
    bs->array = array_create_from_pointer(indices, 1, datalen - 256 * sizeof(frequency_t) - 4);
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

    // FIXME: abstraction
    unsigned char* outdata = (unsigned char*) realloc(out_array->base, out_array->len);
    *outdatalen = out_array->len;

    free(out_array);

    return outdata;
}
