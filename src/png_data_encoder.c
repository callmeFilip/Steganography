#include <stdlib.h>
#include <stdbool.h>

#include "../inc/png_data_encoder.h"
#include "../inc/png_filtration.h"

#include <string.h>

#define BITS_IN_BYTE 8

bool encode_data_rgba(RGBA_pixel **const image, const IHDR_chunk ihdr,
                      unsigned char *const data, uint32_t data_length)
{
    long unsigned int image_size = ihdr.m_width * ihdr.m_height;

    // Check if image is big enough to hold the data
    if ((data_length + HEADER_DATA_LEN) >= ((image_size * RGBA_PIXEL_SIZE) / BITS_IN_BYTE))
    {
        return false;
    }

    // Encode data length
    {
        long int row = 0;
        long int column = 0;

        for (long int data_i = 0, img_i = 0, bit_i = 0; data_i < HEADER_DATA_LEN; data_i++, img_i++)
        {
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            // Set image last bit to 0 and OR it with last bit of data[i],
            // then throw last data bit and replace it with next
            image[row][column].m_red =
                (image[row][column].m_red & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_green =
                (image[row][column].m_green & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_blue =
                (image[row][column].m_blue & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_alpha =
                (image[row][column].m_alpha & 0xFE) | ((data_length >> bit_i++) & 0x01);

            // Go to next pixel, since 2 pixels hold 1 byte total
            img_i++;
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            // Set image last bit to 0 and OR it with last bit of data[i],
            // then throw last data bit and replace it with next
            image[row][column].m_red =
                (image[row][column].m_red & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_green =
                (image[row][column].m_green & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_blue =
                (image[row][column].m_blue & 0xFE) | ((data_length >> bit_i++) & 0x01);

            image[row][column].m_alpha =
                (image[row][column].m_alpha & 0xFE) | ((data_length >> bit_i) & 0x01);

            bit_i = 0;
        }

        // Encode the data
        for (long unsigned int data_i = 0, img_i = HEADER_DATA_LEN * 2, bit_i = 0; data_i < data_length; data_i++, img_i++)
        {
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            // Set image last bit to 0 and OR it with last bit of data[i],
            // then throw last data bit and replace it with next
            image[row][column].m_red =
                (image[row][column].m_red & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_green =
                (image[row][column].m_green & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_blue =
                (image[row][column].m_blue & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_alpha =
                (image[row][column].m_alpha & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            // Go to next pixel, since 2 pixels hold 1 byte total
            img_i++;
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            // Set image last bit to 0 and OR it with last bit of data[i],
            // then throw last data bit and replace it with next
            image[row][column].m_red =
                (image[row][column].m_red & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_green =
                (image[row][column].m_green & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_blue =
                (image[row][column].m_blue & 0xFE) | ((data[data_i] >> bit_i++) & 0x01);

            image[row][column].m_alpha =
                (image[row][column].m_alpha & 0xFE) | ((data[data_i] >> bit_i) & 0x01);

            bit_i = 0;
        }
    }

    return true;
}

bool decode_data_rgba(RGBA_pixel **const image, const IHDR_chunk ihdr,
                      unsigned char **data_in, uint32_t *data_length)
{
    unsigned char *data = NULL;

    // Read data length
    *data_length = 0;

    {
        long int row = 0;
        long int column = 0;
        register long int temp_len = HEADER_DATA_LEN * 2;

        for (long int img_i = 0, bit_i = 0; img_i < temp_len; img_i++)
        {
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            *data_length |= (image[row][column].m_red & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_green & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_blue & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_alpha & 0x01) << bit_i++;

            // Go to next pixel, since 2 pixels hold 1 byte total
            img_i++;
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            *data_length |= (image[row][column].m_red & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_green & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_blue & 0x01) << bit_i++;
            *data_length |= (image[row][column].m_alpha & 0x01) << bit_i;

            bit_i = 0;
        }
    }

    // Allocate buffer
    // Increment length by 1 for '\0' append. Might can be removed in the future
    data = (unsigned char *)malloc(*data_length * sizeof(unsigned char) + 1);

    if (data == NULL)
    {
        return false;
    }

    // Initialize buffer to '\0'
    memset(data, 0, (*data_length) * sizeof(unsigned char) + 1);

    // Read from image to buffer
    {
        long int row = 0;
        long int column = 0;
        register unsigned long int temp_len = (*data_length + HEADER_DATA_LEN) * 2;

        for (unsigned long int img_i = HEADER_DATA_LEN * 2, data_i = 0, bit_i = 0; img_i < temp_len; img_i++, data_i++)
        {
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            data[data_i] |= (image[row][column].m_red & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_green & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_blue & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_alpha & 0x01) << bit_i++;

            // Go to next pixel, since 2 pixels hold 1 byte total
            img_i++;
            row = img_i / ihdr.m_width;
            column = img_i % ihdr.m_width;

            data[data_i] |= (image[row][column].m_red & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_green & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_blue & 0x01) << bit_i++;
            data[data_i] |= (image[row][column].m_alpha & 0x01) << bit_i;

            bit_i = 0;
        }
    }

    (*data_length)++; // Increment length because of '\0' append. Might can be removed in the future

    // Save data state for return
    (*data_in) = data;

    return true;
}
