#include "../inc/codec.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zlib.h"

#include "../inc/global_config.h"
#include "../inc/program_input_parser.h"
#include "../inc/png_parser.h"
#include "../inc/png_filtration.h"
#include "../inc/png_data_encoder.h"

int encoding(program_inp input)
{
    IHDR_chunk ihdr;

    unsigned char *compressed_data = NULL;
    unsigned long int compressed_data_len = 0;

    unsigned char *uncompressed_data = NULL;
    unsigned long uncompressed_data_length = 0;

    RGBA_pixel **unfiltered_data = NULL;

    const char *hidden_data = input.m_operation_argument;

    unsigned long int out_filtered_len = 0;
    unsigned char *out_filtered = NULL;

    unsigned long int out_compressed_len = 0;
    unsigned char *out_compressed_data = NULL;

    if (png_open(input.m_input_name, "rb") == false)
    {
        perror("File is not found!\n");
        return PROGRAM_ERROR;
    }

    // Extract IHDR
    ihdr = read_png_IHDR();

    // Extract compressed data
    compressed_data = (unsigned char *)extract_IDAT_raw_all(&compressed_data_len);

    if (compressed_data == NULL)
    {
        perror("Extraction of IDAT raw data failed!\n");
        return PROGRAM_ERROR;
    }

    // Close image
    png_close();

    // Uncompress data
    uncompressed_data = uncompress_data(ihdr, compressed_data, compressed_data_len, &uncompressed_data_length);

    if (uncompressed_data == NULL)
    {
        perror("Uncompression of IDAT raw data failed!\n");
        return PROGRAM_ERROR;
    }

    free(compressed_data);

    // Unfilter data
    unfiltered_data = unfilter_rgba_png(uncompressed_data, ihdr);

    if (unfiltered_data == NULL)
    {
        perror("Could not unfilter image!\n");
        return PROGRAM_ERROR;
    }

    free(uncompressed_data);

    // Encode data in file
    if (encode_data_rgba(unfiltered_data, ihdr, (unsigned char *)hidden_data, strlen(hidden_data)) == false)
    {
        perror("Encoding failed!\n");
        return PROGRAM_ERROR;
    }

    // Filter data
    out_filtered = filter_rgba_png(ihdr, unfiltered_data, &out_filtered_len);

    if (out_filtered == NULL)
    {
        perror("Could not filter output image!");
        return PROGRAM_ERROR;
    }

    for (size_t i = 0; i < ihdr.m_height; i++)
    {
        free(unfiltered_data[i]);
    }

    free(unfiltered_data);

    // Compress it again
    out_compressed_data = compress_data(&out_compressed_len, out_filtered, out_filtered_len);

    if (out_compressed_data == NULL)
    {
        perror("Compression not compress output image!\n");
        return PROGRAM_ERROR;
    }

    free(out_filtered);

    // Write image
    if (png_open(input.m_output_name, "wb") == false)
    {
        perror("Could not open file!\n");
        return PROGRAM_ERROR;
    }

    if (write_png_IHDR(ihdr) == false)
    {
        perror("Could not write IHDR to file!\n");
        return PROGRAM_ERROR;
    }

    if (write_png_IDAT(out_compressed_data, out_compressed_len) == false)
    {
        perror("Could not write IDAT to file!\n");
        return PROGRAM_ERROR;
    }

    if (write_png_IEND() == false)
    {
        perror("Could not write IEND to file!\n");
        return PROGRAM_ERROR;
    }

    if (png_close() == false)
    {
        perror("Could not close file!\n");
        return PROGRAM_ERROR;
    }

    free(out_compressed_data);

    return PROGRAM_OK;
}

int decoding(program_inp input)
{
    IHDR_chunk ihdr;

    unsigned char *compressed_data = NULL;
    unsigned long int compressed_data_len = 0;

    unsigned char *uncompressed_data = NULL;
    unsigned long uncompressed_data_length = 0;

    RGBA_pixel **unfiltered_data = NULL;

    unsigned char *hidden_data = NULL;
    uint32_t hidden_data_len = 0;

    FILE *hidden_data_txt_fp = NULL;

    if (png_open(input.m_input_name, "rb") == false)
    {
        perror("File is not found!\n");
        return PROGRAM_ERROR;
    }

    // Extract IHDR
    ihdr = read_png_IHDR();

    // Extract compressed data
    compressed_data = (unsigned char *)extract_IDAT_raw_all(&compressed_data_len);

    if (compressed_data == NULL)
    {
        perror("Extraction of IDAT raw data failed!\n");
        return PROGRAM_ERROR;
    }

    // Close image
    png_close();

    // Uncompress data
    uncompressed_data = uncompress_data(ihdr, compressed_data, compressed_data_len, &uncompressed_data_length);

    if (uncompressed_data == NULL)
    {
        perror("Uncompression of IDAT raw data failed!\n");
        return PROGRAM_ERROR;
    }

    free(compressed_data);

    // Unfilter data
    unfiltered_data = unfilter_rgba_png(uncompressed_data, ihdr);

    if (unfiltered_data == NULL)
    {
        perror("Could not unfilter image!\n");
        return PROGRAM_ERROR;
    }

    free(uncompressed_data);

    // Decode data in file
    if (decode_data_rgba(unfiltered_data, ihdr, &hidden_data, &hidden_data_len) == false)
    {
        perror("Decoding failed!\n");
        return PROGRAM_ERROR;
    }

    for (size_t i = 0; i < ihdr.m_height; i++)
    {
        free(unfiltered_data[i]);
    }

    free(unfiltered_data);

    // Print data
    hidden_data_txt_fp = fopen(input.m_output_name, "wb");

    if (hidden_data_txt_fp == NULL)
    {
        perror("Could not write data in txt file\n");
        return PROGRAM_ERROR;
    }

    fprintf(hidden_data_txt_fp, "%s\n", hidden_data);

    fclose(hidden_data_txt_fp);

    free(hidden_data);

    return PROGRAM_OK;
}
