#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/global_config.h"
#include "../inc/png_parser.h"

#include "zlib.h"

// Static global variables
static FILE *g_chunk_ptr = NULL;
static bool g_is_image_open = false;

// Macro and other useful functions
#define swap(x, y) \
    x ^= y;        \
    y ^= x;        \
    x ^= y

#ifdef SYSTEM_SMALL_ENDIAN

static inline void change_endianness(void *inp, const size_t len)
{
    for (size_t i = 0; i < len / 2; i++)
    {
        swap(((char *)inp)[i], ((char *)inp)[len - 1 - i]);
    }
}

#else

static inline void change_endianness(void *inp, size_t len)
{
}

#endif

// File pointer manipulation functions

static inline FILE *chunk_pointer_get()
{
    return g_chunk_ptr;
}

static inline void chunk_pointer_reset()
{
    if (g_is_image_open == false)
    {
        return;
    }

    rewind(chunk_pointer_get());
}

static inline void chunk_pointer_set(const long int position)
{
    if (g_is_image_open == false)
    {
        return;
    }

    fseek(chunk_pointer_get(), position, SEEK_SET);
}

// Image open/close control functions

bool png_open(const char *_FileName, const char *_Mode)
{
    g_chunk_ptr = fopen(_FileName, _Mode);

    if (g_chunk_ptr == NULL)
    {
        return g_is_image_open = false;
    }

    return g_is_image_open = true;
}

bool png_close()
{
    if (g_is_image_open == false)
    {
        return false;
    }

    fclose(chunk_pointer_get());
    g_is_image_open = false;

    return true;
}

// Chunk manipulation functions

static outside_chunk get_outside_chunk()
{
    outside_chunk result = {OUTSIDE_CHUNK_DEFAULT_INIT_ARGS};

    if (g_is_image_open == false)
    {
        return result;
    }

    // Save file pointer entry state
    result.m_entry_point = ftell(chunk_pointer_get());

    // Read data length
    fread(&result.m_data_length, sizeof(result.m_data_length), 1, chunk_pointer_get());
    change_endianness(&result.m_data_length, sizeof(result.m_data_length));

    // Read chunk type
    fread(&result.m_type, sizeof(result.m_type), 1, chunk_pointer_get());

    // Skip to chunk end
    if (result.m_data_length != 0)
    {
        fseek(chunk_pointer_get(), result.m_data_length, SEEK_CUR);
    }

    // Read chunk CRC-32
    fread(&result.m_CRC_32, sizeof(result.m_CRC_32), 1, chunk_pointer_get());
    change_endianness(&result.m_CRC_32, sizeof(result.m_CRC_32));

    // Reset to entry state
    chunk_pointer_set(result.m_entry_point);

    return result;
}

static outside_chunk chunk_seek(const unsigned char *const chunk_signature, bool is_reset)
{
    static bool is_at_end = false;
    static long int last_address = HEADER_LENGTH; // Skip file signature chunk

    outside_chunk curr_chunk = {OUTSIDE_CHUNK_DEFAULT_INIT_ARGS};

    if (g_is_image_open == false)
    {
        return curr_chunk;
    }

    if (is_reset == PNG_PARSER_RESET)
    {
        // Skip file signature chunk
        last_address = HEADER_LENGTH;

        // Reset end flag
        is_at_end = false;
    }
    else if (is_at_end == true)
    {
        return curr_chunk;
    }

    // Save file pointer entry state
    long int entry_point = ftell(chunk_pointer_get());

    // Set to end of last chunk
    chunk_pointer_set(last_address);

    do // Start seeking
    {
        // Stop seeking if last chunk was IEND
        if (memcmp(curr_chunk.m_type, IEND_SIGNATURE, TYPE_SIGNATURE_LENGTH) == 0)
        {
            is_at_end = true;
            break;
        }

        // Read chunk_outside_data
        curr_chunk = get_outside_chunk(chunk_pointer_get());

        // Go to next chunk
        chunk_pointer_set(curr_chunk.m_entry_point + HEADER_LENGTH +
                          FOOTER_LENGTH + curr_chunk.m_data_length);

        // Save end of last chunk
        last_address = ftell(chunk_pointer_get());

    } while (memcmp(curr_chunk.m_type, chunk_signature, TYPE_SIGNATURE_LENGTH) != 0);

    // Reset to entry state
    chunk_pointer_set(entry_point);

    return curr_chunk;
}

IHDR_chunk read_png_IHDR()
{
    IHDR_chunk IHDR = {0};

    if (g_is_image_open == false)
    {
        return IHDR;
    }

    // Find IHDR
    IHDR.m_outside_chunk = chunk_seek(IHDR_SIGNATURE, PNG_PARSER_RESET);

    // Save file pointer entry state
    long int entry_point = ftell(chunk_pointer_get());

    // Set ptr to start of IHDR data chunk
    chunk_pointer_set(IHDR.m_outside_chunk.m_entry_point + HEADER_LENGTH);

    // Read everything at once
    fread(&IHDR.m_width, sizeof(uint32_t), 1, chunk_pointer_get());
    fread(&IHDR.m_height, sizeof(uint32_t), 1, chunk_pointer_get());
    fread(&IHDR.m_bit_depth, sizeof(char), 1, chunk_pointer_get());
    fread(&IHDR.m_color_type, sizeof(char), 1, chunk_pointer_get());
    fread(&IHDR.m_compression_method, sizeof(char), 1, chunk_pointer_get());
    fread(&IHDR.m_filter_method, sizeof(char), 1, chunk_pointer_get());
    fread(&IHDR.m_interlace_method, sizeof(char), 1, chunk_pointer_get());

    // Cast data from Big endian to Small endian
    change_endianness(&IHDR.m_width, sizeof(IHDR.m_width));
    change_endianness(&IHDR.m_height, sizeof(IHDR.m_height));

    // Reset to entry state
    chunk_pointer_set(entry_point);

    return IHDR;
}

IDAT_chunk read_png_IDAT(const bool is_reset)
{
    IDAT_chunk IDAT = {0, {OUTSIDE_CHUNK_DEFAULT_INIT_ARGS}};

    if (g_is_image_open == false)
    {
        return IDAT;
    }

    // Find IDAT
    IDAT.m_outside_chunk = chunk_seek(IDAT_SIGNATURE, is_reset);

    // Set inside chunk data starting address
    if (IDAT.m_outside_chunk.m_data_length != 0)
    {
        IDAT.m_raw_inside_chunk_data_offset = IDAT.m_outside_chunk.m_entry_point + HEADER_LENGTH;
    }

    return IDAT;
}

unsigned char *extract_IDAT_raw(const long int address, const uint32_t length)
{
    unsigned char *result = NULL;

    if (g_is_image_open == false)
    {
        return result;
    }

    result = (unsigned char *)malloc(length * sizeof(unsigned char));

    if (result == NULL)
    {
        return NULL;
    }

    // Save file pointer entry state
    long int entry_point = ftell(chunk_pointer_get());

    // Move pointer to start of data
    chunk_pointer_set(address);

    // Copy data from file to IDAT data chunk storage
    fread(result, sizeof(unsigned char), length, chunk_pointer_get());

    // Reset to entry state
    chunk_pointer_set(entry_point);

    return result;
}

unsigned char *extract_IDAT_raw_all(unsigned long int *total_compressed_data_length)
{
    IDAT_chunk idat;

    unsigned char *result = NULL;
    unsigned char *temp_placeholder = NULL;

    uint32_t first_free_index = 0;
    *total_compressed_data_length = 0;

    if (g_is_image_open == false)
    {
        return result;
    }

    // Get first IDAT chunk
    idat = read_png_IDAT(PNG_PARSER_RESET);
    *total_compressed_data_length = idat.m_outside_chunk.m_data_length;

    // Initial allocation
    result = extract_IDAT_raw(idat.m_raw_inside_chunk_data_offset, idat.m_outside_chunk.m_data_length);

    if (result == NULL)
    {
        return NULL;
    }

    while (memcmp(idat.m_outside_chunk.m_type, IEND_SIGNATURE, TYPE_SIGNATURE_LENGTH) != 0)
    {

        // Save one beyond last index for later usage
        first_free_index = *total_compressed_data_length;

        // Read chunk data
        idat = read_png_IDAT(PNG_PARSER_NEXT);

        // If current IDAT chunk contains data:
        if (idat.m_outside_chunk.m_data_length > 0)
        {
            // Increase buffer size
            *total_compressed_data_length += idat.m_outside_chunk.m_data_length;
            result = realloc(result, *total_compressed_data_length);

            if (result == NULL)
            {
                return NULL;
            }

            // Temporary allocates memory for new IDAT data
            temp_placeholder = extract_IDAT_raw(idat.m_raw_inside_chunk_data_offset, idat.m_outside_chunk.m_data_length);

            if (temp_placeholder == NULL)
            {
                return NULL;
            }

            // Copy IDAT data to buffer
            memcpy(&result[first_free_index], temp_placeholder, idat.m_outside_chunk.m_data_length);

            // Clean temporary allocated memory
            free(temp_placeholder);
        }
    }

    return result;
}

unsigned char *uncompress_data(const IHDR_chunk ihdr, const Bytef *const c_d_buffer, const uLong c_d_length, uLongf *u_d_length)
{
    Bytef *result = NULL; // Uncompressed data buffer
    *u_d_length = 0;

    // Works only for color type RGBA(6)
    if (ihdr.m_color_type != COLOR_TYPE_RGBA)
    {
        return NULL;
    }

    // Calculate length for uncompressed data buffer. Size of image * Pixels in RGBA format + filter type markers
    *u_d_length = ihdr.m_width * ihdr.m_height * RGBA_PIXEL_SIZE * sizeof(Bytef) + ihdr.m_height;

    // Allocate storage
    result = (Bytef *)malloc(*u_d_length);

    if (result == NULL)
    {
        return NULL;
    }

    // Call to zlib uncompress
    if (uncompress(result, u_d_length, c_d_buffer, c_d_length) != Z_OK)
    {
        return NULL;
    }

    return result;
}

unsigned char *compress_data(uLong *c_d_length, const Bytef *const u_d_buffer, uLongf u_d_length)
{
    unsigned char *result = NULL; // Compressed data buffer

    // Calculate length for compressed data buffer
    *c_d_length = compressBound(u_d_length);

    // Allocate storage
    result = (unsigned char *)malloc(*c_d_length);

    if (result == NULL)
    {
        return NULL;
    }

    // Call to zlib compress
    if (compress(result, c_d_length, u_d_buffer, u_d_length) != Z_OK)
    {
        return NULL;
    }

    return result;
}

bool write_png_IHDR(IHDR_chunk ihdr)
{
    uint32_t width = ihdr.m_width;
    uint32_t height = ihdr.m_height;
    uint32_t ihdr_len = ihdr.m_outside_chunk.m_data_length;
    uint32_t ihdr_crc32 = ihdr.m_outside_chunk.m_CRC_32;

#ifdef SYSTEM_SMALL_ENDIAN

    change_endianness(&width, sizeof(uint32_t));
    change_endianness(&height, sizeof(uint32_t));
    change_endianness(&ihdr_len, sizeof(uint32_t));
    change_endianness(&ihdr_crc32, sizeof(uint32_t));

#endif

    if (g_is_image_open == false)
    {
        return false;
    }

    // Reset to start of file
    chunk_pointer_reset();

    // Write file signature
    fwrite(FILE_SIGNATURE, FILE_SIGNATURE_LENGTH, 1, chunk_pointer_get());

    // Write IHDR data
    fwrite(&ihdr_len, HEADER_DATA_LEN, 1, chunk_pointer_get());
    fwrite(&ihdr.m_outside_chunk.m_type, sizeof(char), HEADER_TYPE_LEN, chunk_pointer_get());
    fwrite(&width, sizeof(uint32_t), 1, chunk_pointer_get());
    fwrite(&height, sizeof(uint32_t), 1, chunk_pointer_get());
    fwrite(&ihdr.m_bit_depth, sizeof(unsigned char), 1, chunk_pointer_get());
    fwrite(&ihdr.m_color_type, sizeof(unsigned char), 1, chunk_pointer_get());
    fwrite(&ihdr.m_compression_method, sizeof(unsigned char), 1, chunk_pointer_get());
    fwrite(&ihdr.m_filter_method, sizeof(unsigned char), 1, chunk_pointer_get());
    fwrite(&ihdr.m_interlace_method, sizeof(unsigned char), 1, chunk_pointer_get());
    fwrite(&ihdr_crc32, sizeof(uint32_t), 1, chunk_pointer_get());

    return true;
}

bool write_png_IDAT(const unsigned char *const raw_data, const unsigned long int raw_data_len)
{
    uint32_t idat_len = raw_data_len;
    uint32_t crc = 0;

    unsigned char *temp_buf = NULL;

#ifdef SYSTEM_SMALL_ENDIAN

    change_endianness(&idat_len, sizeof(uint32_t));

#endif

    if (g_is_image_open == false)
    {
        return false;
    }

    // Seed CRC-32
    crc = crc32(0L, Z_NULL, 0);

    // Temporary store raw data and chunk type in the same place
    temp_buf = malloc((raw_data_len + HEADER_DATA_LEN) * sizeof(unsigned char));

    if (temp_buf == NULL)
    {
        return false;
    }

    memcpy(temp_buf, IDAT_SIGNATURE, HEADER_TYPE_LEN);
    memcpy(temp_buf + HEADER_DATA_LEN, raw_data, raw_data_len);

    // Calculate CRC-32
    crc = crc32(crc, temp_buf, raw_data_len + HEADER_DATA_LEN);

    // Free memory
    free(temp_buf);

#ifdef SYSTEM_SMALL_ENDIAN

    change_endianness(&crc, sizeof(uint32_t));

#endif

    // Write header
    fwrite(&idat_len, HEADER_DATA_LEN, 1, chunk_pointer_get());
    fwrite(IDAT_SIGNATURE, HEADER_TYPE_LEN, 1, chunk_pointer_get());

    // Write data
    fwrite(raw_data, 1, raw_data_len, chunk_pointer_get());

    // Write CRC-32
    fwrite(&crc, FOOTER_LENGTH, 1, chunk_pointer_get());

    return true;
}

bool write_png_IEND()
{
    uint32_t iend_data_len = 0;

    if (g_is_image_open == false)
    {
        return false;
    }

    // Write IEND data
    fwrite(&iend_data_len, HEADER_DATA_LEN, 1, chunk_pointer_get());
    fwrite(IEND_SIGNATURE, HEADER_TYPE_LEN, 1, chunk_pointer_get());
    fwrite(IEND_CRC_32, FOOTER_LENGTH, 1, chunk_pointer_get());

    return true;
}
