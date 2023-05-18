#ifndef PNG_FILTRATION_H
#define PNG_FILTRATION_H

#include "stdint.h"

#include "../inc/png_parser.h"

/**
 * @brief Single RGBA pixel struct.
 */
typedef struct
{
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
    unsigned char m_alpha;

} RGBA_pixel;

/**
 * @brief Produce image matrix from filtered buffer
 *
 * @param filtered_buffer Sequential input buffer with filtered image
 * @param ihdr IHDR of the filtered image
 * @return RGBA_pixel** 2D Image array
 */
RGBA_pixel **unfilter_rgba_png(const unsigned char *const filtered_buffer, const IHDR_chunk ihdr);

/**
 * @brief Produce filtered image buffer, ready to get compressed
 *
 * @param ihdr IHDR of the unfiltered image
 * @param unfiltered_image 2D Image array
 * @param length Outputs the length of the filtered buffer
 * @return Filtered image buffer
 */
unsigned char *filter_rgba_png(const IHDR_chunk ihdr, RGBA_pixel **unfiltered_image, unsigned long int *const length);

#endif // ~PNG_FILTRATION_H
