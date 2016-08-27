#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
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
        return -1;
    else if(*freq2 == NULL)
        return 1;

    if((*freq1)->count < (*freq2)->count)
        return -1;
    else if((*freq1)->count > (*freq2)->count)
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
        list[i] = freqs + i;
        listLen++;
    }

    for(int i = 256; i < 1024; i++)
    {
        list[i] = NULL;
    }

    // FIXME FIXME FIXME: memory leak
    frequency_t* internalNodes = (frequency_t*) calloc(1024, sizeof(frequency_t));
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

    internalNodes = (frequency_t*) realloc(internalNodes, sizeof(frequency_t*) * internalNodePos);

    frequency_t* retval = list[0];
    free(list);

    return retval;
}

/*
string encodeChar(node_t root, char c, string acc = "")
{
	if(c == root.val)
	{
		return acc;
	}
	else if(root.left == null)
	{
		return null;
	}

	return encodeChar(root.left, c, acc + "0") ?? encodeChar(root.right, c, acc + "1");
}
*/
array_t* huffman_encode_byte(frequency_t* root, char byte, array_t* acc)
{
    if(root->val == byte)
    {
        return acc;
    }
    else if(root->left == NULL)
    {
        return NULL;
    }

    // FIXME: shitloads of memory leaks
    unsigned char left_c = 0x00;
    array_t* left_acc = array_copy(acc);
    array_append(left_acc, &left_c);
    array_t* left_result = huffman_encode_byte(root->left, byte, left_acc);
    if(left_result != null)
    {
        return left_result;
    }
    free(left_acc);

    unsigned char right_c = 0x01;
    array_t* right_acc = array_copy(acc);
    array_append(right_acc, &right_c);
    array_t* right_result = huffman_encode_byte(root->right, byte, right_acc);
    if(right_result != null)
    {
        return right_result;
    }
    free(right_acc);

    // FIXME: don't think it can ever reach here, but I might be wrong
    return NULL;
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
    array_t* outbitstream = array_create(1, 6);

    // write frequencies, so we can build dictionary
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

/*
string decodeChars(string s, node_t realroot)
{
	node_t root = realroot;
	StringBuilder acc = new StringBuilder();
	int spos = 0;

	while(spos < s.Length)
	{
		if(root.left == null)
		{
			acc.Append(root.val);
			root = realroot;
		}

		if(s[spos] == '0')
		{
			root = root.left;
			spos++;
		}
		else
		{
			root = root.right;
			spos++;
		}
	}

	acc.Append(root.val);

	return acc.ToString();
}
*/
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
