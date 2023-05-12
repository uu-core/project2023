#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void get_walsh_codes(uint8_t num_codes, uint64_t buf[num_codes])
{
    /* Check if num_codes is a power of two */
    assert((num_codes != 0) && ((num_codes & (num_codes - 1)) == 0) || num_codes > 64);

    uint8_t matrix[num_codes][num_codes];
    uint8_t tmp_matrix[num_codes][num_codes];

    /* Clear input buffer. */
    memset(buf, 0, num_codes * sizeof(uint64_t));
    memset(matrix, 0, num_codes * num_codes * sizeof(uint8_t));
    memset(tmp_matrix, 0, num_codes * num_codes * sizeof(uint8_t));

    /* Initial Walsh matrix */
    tmp_matrix[0][0] = 0;
    tmp_matrix[0][1] = 0;
    tmp_matrix[1][0] = 0;
    tmp_matrix[1][1] = 1;

    for (uint8_t size = 2; size < num_codes; size *= 2)
    {
        /* XOR the initial walsh matrix values with another initial walsh matrix
        to spread the values */
        for (uint8_t j = 0; j < size; j++)
        {
            for (uint8_t k = 0; k < size; k++)
            {
                matrix[j][k] = tmp_matrix[j][k] ^ 0;
                matrix[j + size][k] = tmp_matrix[j][k] ^ 0;
                matrix[j][k + size] = tmp_matrix[j][k] ^ 0;
                matrix[j + size][k + size] = tmp_matrix[j][k] ^ 1;
            }
        }

        /* Copy over the temporary matrix so that we can do the next step. */
        memcpy(tmp_matrix, matrix, sizeof(matrix));
    }

    /* We pad the 2 dimensional matrix onto a one dimensional array to be used
    instead. Each element repreesent a walsh code. */
    for (uint8_t i = 0; i < num_codes; i++)
    {
        for (uint8_t j = 0; j < num_codes; j++)
        {
            buf[i] |= (matrix[i][j] << (num_codes - j - 1));
        }
    }
}