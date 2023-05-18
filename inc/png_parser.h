#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#include <stdbool.h>

#include "stdint.h"

// Control keywords
#define PNG_PARSER_RESET true
#define PNG_PARSER_NEXT false

// Signature configurations
#define HEADER_DATA_LEN 4
#define HEADER_TYPE_LEN 4
#define HEADER_LENGTH HEADER_DATA_LEN + HEADER_TYPE_LEN
#define FOOTER_LENGTH 4
#define TYPE_SIGNATURE_LENGTH 4
#define FILE_SIGNATURE_LENGTH HEADER_LENGTH

static const unsigned char FILE_SIGNATURE[FILE_SIGNATURE_LENGTH] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
static const unsigned char IDAT_SIGNATURE[TYPE_SIGNATURE_LENGTH] = {0x49, 0x44, 0x41, 0x54};
static const unsigned char IHDR_SIGNATURE[TYPE_SIGNATURE_LENGTH] = {0x49, 0x48, 0x44, 0x52};
static const unsigned char sRGB_SIGNATURE[TYPE_SIGNATURE_LENGTH] = {0x73, 0x52, 0x47, 0x42};
static const unsigned char IEND_SIGNATURE[TYPE_SIGNATURE_LENGTH] = {0x49, 0x45, 0x4e, 0x44};
static const unsigned char IEND_CRC_32[FOOTER_LENGTH] = {0xae, 0x42, 0x60, 0x82};

// Default initialization values
#define NULL_SIGNATURE "NULL"
#define OUTSIDE_CHUNK_DEFAULT_INIT_ARGS 0, NULL_SIGNATURE, 0, 0

// Constants
#define RGBA_PIXEL_SIZE 4

// Color types
#define COLOR_TYPE_RGBA 6

/**
 * @brief Structure that hold ouside chunk data
 *
 * m_type defaults to "NULL"
 */
typedef struct
{
    uint32_t m_data_length;
    unsigned char m_type[4];
    uint32_t m_CRC_32;
    long int m_entry_point;

} outside_chunk;

/**
 * @brief Struct that hold IHDR data
 */
typedef struct
{
    uint32_t m_width;
    uint32_t m_height;
    unsigned char m_bit_depth;
    unsigned char m_color_type;
    unsigned char m_compression_method;
    unsigned char m_filter_method;
    unsigned char m_interlace_method;

    outside_chunk m_outside_chunk;

} IHDR_chunk;

/**
 * @brief Struct that hold IDAT data
 *
 * m_raw_inside_chunk_data_offset holds relative address
 * (offset from start of file)
 */
typedef struct
{
    long int m_raw_inside_chunk_data_offset;

    outside_chunk m_outside_chunk;

} IDAT_chunk;

/**
 * @brief Open file (fopen)
 *
 * @param _FileName Path to png file
 * @param _Mode fopen mode
 * @return True if successful, false if not
 */
bool png_open(const char *_FileName, const char *_Mode);

/**
 * @brief Close file (fclose)
 *
 * @return True if successful, false if not
 */
bool png_close();

/**
 * @brief Reads the IHDR
 *
 * @return IHDR_chunk
 */
IHDR_chunk read_png_IHDR();

/**
 * @brief Reads IDAT chunk
 *
 * @param is_reset If PNG_PARSER_RESET is given it reads the
 * first IDAT chunk, else if PNG_PARSER_NEXT is given it reads
 * the IDAT chunk after the last read chunk
 * @return IDAT_chunk
 */
IDAT_chunk read_png_IDAT(const bool is_reset);

/**
 * @brief Extract raw data from IDAT chunk
 *
 * @param address Virtual address/offset from start of file (0)
 * @param length Length of data
 * @return Dynamically allocated unsigned char* buffer with raw IDAT data
 */
unsigned char *extract_IDAT_raw(const long int address, const uint32_t length);

/**
 * @brief Extracts all IDAT data in PNG
 *
 * @param total_compressed_data_length Gives length of data buffer
 * @return Pointer to compressed data buffer
 */
unsigned char *extract_IDAT_raw_all(unsigned long int *total_compressed_data_length);

/**
 * @brief Uncompress data
 *
 * @param ihdr IHDR data
 * @param compressed_data_buffer Pointer to source of compressed data
 * @param compressed_data_length Length of compressed data
 * @param uncompressed_data_length Gives length of uncompressed data
 * @return Pointer to uncompressed data buffer
 */
unsigned char *uncompress_data(const IHDR_chunk ihdr, const unsigned char *const compressed_data_buffer,
                               const unsigned long int compressed_data_length, unsigned long int *uncompressed_data_length);

/**
 * @brief Compresses data
 *
 * @param compressed_data_length Gives length of compressed data
 * @param uncompressed_data_buffer Pointer to source of uncompressed data
 * @param uncompressed_data_length Length of uncompressed data
 * @return Pointer to compressed data buffer
 */
unsigned char *compress_data(unsigned long int *compressed_data_length, const unsigned char *const uncompressed_data_buffer,
                             const unsigned long int uncompressed_data_length);

/**
 * @brief Write PNG IHDR chunk at current file pointer state
 *
 * @param ihdr IHDR of image
 * @return True if successful, false if not
 */
bool write_png_IHDR(IHDR_chunk ihdr);

/**
 * @brief Write PNG chunk data at current file pointer state
 *
 * @param buffer Buffer with IDAT data
 * @param buf_length Length of IDAT buffer
 * @return True if successful, false if not
 */
bool write_png_IDAT(const unsigned char *const buffer, const unsigned long int buf_length);

/**
 * @brief Write PNG IEND chunk at current file pointer state
 *
 * @return True if successful, false if not
 */
bool write_png_IEND();

#endif // ~PNG_PARSER_H
