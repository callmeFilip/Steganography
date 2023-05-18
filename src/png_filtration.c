#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "stdint.h"

#include "../inc/png_filtration.h"
#include "../inc/png_parser.h"

// Defines of filters for filter method(0)
#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH 4
#define PNG_FILTER_COUNT 5

// Reverse RGBA filtering functions

static int paeth_predictor(const unsigned char l, const unsigned char u, const unsigned char ul)
{
    // Calculate coefficients
    int p = (l + u - ul);

    int pl = abs(p - l);
    int pu = abs(p - u);
    int pul = abs(p - ul);

    // Return based on min coefficient
    if (pl <= pu && pl <= pul)
    {
        return l;
    }
    else if (pu <= pul)
    {
        return u;
    }
    else
    {
        return ul;
    }
}

static void reverse_rgba_png_filter_none(const unsigned char *const src, RGBA_pixel *const dest, const uint32_t width)
{
    // Recon(x) = Filt(x)

    for (size_t i = 0; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i].m_red = src[i * RGBA_PIXEL_SIZE + 1];
        dest[i].m_green = src[i * RGBA_PIXEL_SIZE + 2];
        dest[i].m_blue = src[i * RGBA_PIXEL_SIZE + 3];
        dest[i].m_alpha = src[i * RGBA_PIXEL_SIZE + 4];
    }
}

static void reverse_rgba_png_filter_sub(const unsigned char *const src, RGBA_pixel *const dest, const uint32_t width)
{
    // Recon(x) = (Filt(x) + Recon(l)) % 256

    // Set most left bytes (the element before start of row is 0)
    dest[0].m_red = src[1];
    dest[0].m_green = src[2];
    dest[0].m_blue = src[3];
    dest[0].m_alpha = src[4];

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i].m_red = (src[i * RGBA_PIXEL_SIZE + 1] + dest[i - 1].m_red) % 256;
        dest[i].m_green = (src[i * RGBA_PIXEL_SIZE + 2] + dest[i - 1].m_green) % 256;
        dest[i].m_blue = (src[i * RGBA_PIXEL_SIZE + 3] + dest[i - 1].m_blue) % 256;
        dest[i].m_alpha = (src[i * RGBA_PIXEL_SIZE + 4] + dest[i - 1].m_alpha) % 256;
    }
}

static void reverse_rgba_png_filter_up(const unsigned char *const src, const RGBA_pixel *const prev, RGBA_pixel *const dest, const uint32_t width)
{
    // Recon(x) = (Filt(x) + Recon(u)) % 256

    for (size_t i = 0; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i].m_red = (src[i * RGBA_PIXEL_SIZE + 1] + prev[i].m_red) % 256;
        dest[i].m_green = (src[i * RGBA_PIXEL_SIZE + 2] + prev[i].m_green) % 256;
        dest[i].m_blue = (src[i * RGBA_PIXEL_SIZE + 3] + prev[i].m_blue) % 256;
        dest[i].m_alpha = (src[i * RGBA_PIXEL_SIZE + 4] + prev[i].m_alpha) % 256;
    }
}

static void reverse_rgba_png_filter_average(const unsigned char *const src, const RGBA_pixel *const prev, RGBA_pixel *const dest, const uint32_t width)
{
    // Recon(x) = (Filt(x) + floor((Recon(l) + Recon(u)) / 2)) % 256

    // Set most left bytes (the elements before start of row and on top of first row are 0)
    dest[0].m_red = (src[1] + prev[0].m_red / 2) % 256;
    dest[0].m_green = (src[2] + prev[0].m_green / 2) % 256;
    dest[0].m_blue = (src[3] + prev[0].m_blue / 2) % 256;
    dest[0].m_alpha = (src[4] + prev[0].m_alpha / 2) % 256;

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i].m_red = (src[i * RGBA_PIXEL_SIZE + 1] + (dest[i - 1].m_red + prev[i].m_red) / 2) % 256;
        dest[i].m_green = (src[i * RGBA_PIXEL_SIZE + 2] + (dest[i - 1].m_green + prev[i].m_green) / 2) % 256;
        dest[i].m_blue = (src[i * RGBA_PIXEL_SIZE + 3] + (dest[i - 1].m_blue + prev[i].m_blue) / 2) % 256;
        dest[i].m_alpha = (src[i * RGBA_PIXEL_SIZE + 4] + (dest[i - 1].m_alpha + prev[i].m_alpha) / 2) % 256;
    }
}

static void reverse_rgba_png_filter_paeth(const unsigned char *const src, const RGBA_pixel *const prev, RGBA_pixel *const dest, const uint32_t width)
{
    // Recon(x) = (Filt(x) + PaethPredictor(Recon(l), Recon(u), Recon(ul))) % 256

    // Set most left bytes (the elements before start of row and on top left of first row are 0)
    dest[0].m_red = (src[1] + paeth_predictor(0, prev[0].m_red, 0)) % 256;
    dest[0].m_green = (src[2] + paeth_predictor(0, prev[0].m_green, 0)) % 256;
    dest[0].m_blue = (src[3] + paeth_predictor(0, prev[0].m_blue, 0)) % 256;
    dest[0].m_alpha = (src[4] + paeth_predictor(0, prev[0].m_alpha, 0)) % 256;

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i].m_red = (src[i * RGBA_PIXEL_SIZE + 1] +
                         paeth_predictor(dest[i - 1].m_red, prev[i].m_red, prev[i - 1].m_red)) %
                        256;
        dest[i].m_green = (src[i * RGBA_PIXEL_SIZE + 2] +
                           paeth_predictor(dest[i - 1].m_green, prev[i].m_green, prev[i - 1].m_green)) %
                          256;
        dest[i].m_blue = (src[i * RGBA_PIXEL_SIZE + 3] +
                          paeth_predictor(dest[i - 1].m_blue, prev[i].m_blue, prev[i - 1].m_blue)) %
                         256;
        dest[i].m_alpha = (src[i * RGBA_PIXEL_SIZE + 4] +
                           paeth_predictor(dest[i - 1].m_alpha, prev[i].m_alpha, prev[i - 1].m_alpha)) %
                          256;
    }
}

// Apply RGBA filtering functions

static void apply_rgba_png_filter_none(const RGBA_pixel *const src, unsigned char *const dest, const uint32_t width)
{
    // Filt(x) = Recon(x)
    dest[0] = PNG_FILTER_NONE; // Set filter to None

    for (size_t i = 0; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i * RGBA_PIXEL_SIZE + 1] = src[i].m_red;
        dest[i * RGBA_PIXEL_SIZE + 2] = src[i].m_green;
        dest[i * RGBA_PIXEL_SIZE + 3] = src[i].m_blue;
        dest[i * RGBA_PIXEL_SIZE + 4] = src[i].m_alpha;
    }
}

static void apply_rgba_png_filter_sub(const RGBA_pixel *const src, unsigned char *const dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - Orig(l)) % 256;
    dest[0] = PNG_FILTER_SUB; // Set filter to Sub

    // Set most left bytes (the element before start of row is 0)
    dest[1] = src[0].m_red;
    dest[2] = src[0].m_green;
    dest[3] = src[0].m_blue;
    dest[4] = src[0].m_alpha;

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i * RGBA_PIXEL_SIZE + 1] = (src[i].m_red - src[i - 1].m_red) % 256;
        dest[i * RGBA_PIXEL_SIZE + 2] = (src[i].m_green - src[i - 1].m_green) % 256;
        dest[i * RGBA_PIXEL_SIZE + 3] = (src[i].m_blue - src[i - 1].m_blue) % 256;
        dest[i * RGBA_PIXEL_SIZE + 4] = (src[i].m_alpha - src[i - 1].m_alpha) % 256;
    }
}

static void apply_rgba_png_filter_up(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - Orig(u)) % 256
    dest[0] = PNG_FILTER_UP; // Set filter to Up

    for (size_t i = 0; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i * RGBA_PIXEL_SIZE + 1] = (src[i].m_red - prev[i].m_red) % 256;
        dest[i * RGBA_PIXEL_SIZE + 2] = (src[i].m_green - prev[i].m_green) % 256;
        dest[i * RGBA_PIXEL_SIZE + 3] = (src[i].m_blue - prev[i].m_blue) % 256;
        dest[i * RGBA_PIXEL_SIZE + 4] = (src[i].m_alpha - prev[i].m_alpha) % 256;
    }
}

static void apply_rgba_png_filter_average(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - floor((Orig(l) + Orig(u)) / 2)) % 256
    dest[0] = PNG_FILTER_AVERAGE; // Set filter to Average

    // Set most left bytes (the element before start of row is 0)
    dest[1] = (src[0].m_red - prev[0].m_red / 2) % 256;
    dest[2] = (src[0].m_green - prev[0].m_green / 2) % 256;
    dest[3] = (src[0].m_blue - prev[0].m_blue / 2) % 256;
    dest[4] = (src[0].m_alpha - prev[0].m_alpha / 2) % 256;

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i * RGBA_PIXEL_SIZE + 1] = (src[i].m_red - (src[i - 1].m_red + prev[i].m_red) / 2) % 256;
        dest[i * RGBA_PIXEL_SIZE + 2] = (src[i].m_green - (src[i - 1].m_green + prev[i].m_green) / 2) % 256;
        dest[i * RGBA_PIXEL_SIZE + 3] = (src[i].m_blue - (src[i - 1].m_blue + prev[i].m_blue) / 2) % 256;
        dest[i * RGBA_PIXEL_SIZE + 4] = (src[i].m_alpha - (src[i - 1].m_alpha + prev[i].m_alpha) / 2) % 256;
    }
}

static void apply_rgba_png_filter_paeth(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - PaethPredictor(Orig(l), Orig(u), Orig(ul))) % 256
    dest[0] = PNG_FILTER_PAETH; // Set filter to Paeth

    // Set most left bytes (the elements before start of row and on top left of first row are 0)
    dest[1] = (src[0].m_red - paeth_predictor(0, prev[0].m_red, 0)) % 256;
    dest[2] = (src[0].m_green - paeth_predictor(0, prev[0].m_green, 0)) % 256;
    dest[3] = (src[0].m_blue - paeth_predictor(0, prev[0].m_blue, 0)) % 256;
    dest[4] = (src[0].m_alpha - paeth_predictor(0, prev[0].m_alpha, 0)) % 256;

    for (size_t i = 1; i < width; i++)
    {
        // + 1 in src is for skipping the filter byte
        dest[i * RGBA_PIXEL_SIZE + 1] = (src[i].m_red -
                                         paeth_predictor(src[i - 1].m_red, prev[i].m_red, prev[i - 1].m_red)) %
                                        256;
        dest[i * RGBA_PIXEL_SIZE + 2] = (src[i].m_green -
                                         paeth_predictor(src[i - 1].m_green, prev[i].m_green, prev[i - 1].m_green)) %
                                        256;
        dest[i * RGBA_PIXEL_SIZE + 3] = (src[i].m_blue -
                                         paeth_predictor(src[i - 1].m_blue, prev[i].m_blue, prev[i - 1].m_blue)) %
                                        256;
        dest[i * RGBA_PIXEL_SIZE + 4] = (src[i].m_alpha -
                                         paeth_predictor(src[i - 1].m_alpha, prev[i].m_alpha, prev[i - 1].m_alpha)) %
                                        256;
    }
}

// Apply heuristics filtering functions

static void apply_heur_png_filter_none(const RGBA_pixel *const src, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = Recon(x)
    dest[0] = PNG_FILTER_NONE; // Set filter to None
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    for (size_t i = 0; i < temp_len; i++)
    {
        // + 1 in dest is for skipping the filter byte
        dest[i + 1] = ((unsigned char *)src)[i];
    }
}

static void apply_heur_png_filter_sub(const RGBA_pixel *const src, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - Orig(l)) % 256
    dest[0] = PNG_FILTER_SUB; // Set filter to Sub

    // Set most left byte (the element before start of row is 0)
    dest[1] = ((unsigned char *)src)[0];
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    for (size_t i = 1; i < temp_len; i++)
    {
        // + 1 in dest is for skipping the filter byte
        dest[i + 1] = (((unsigned char *)src)[i] - ((unsigned char *)src)[i - 1]) % 256;
    }
}

static void apply_heur_png_filter_up(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - Orig(u)) % 256
    dest[0] = PNG_FILTER_UP; // Set filter to Up
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    for (size_t i = 0; i < temp_len; i++)
    {
        // + 1 in dest is for skipping the filter byte
        dest[i + 1] = (((unsigned char *)src)[i] - ((unsigned char *)prev)[i]) % 256;
    }
}

static void apply_heur_png_filter_average(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - floor((Orig(l) + Orig(u)) / 2)) % 256
    dest[0] = PNG_FILTER_AVERAGE; // Set filter to Average

    // Set most left byte (the element before start of row is 0)
    dest[1] = (((unsigned char *)src)[0] - ((unsigned char *)prev)[0] / 2) % 256;
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    for (size_t i = 1; i < temp_len; i++)
    {
        // + 1 in dest is for skipping the filter byte
        dest[i + 1] = (((unsigned char *)src)[i] - ((((unsigned char *)src)[i - 1] + ((unsigned char *)prev)[i]) / 2)) % 256;
    }
}

static void apply_heur_png_filter_paeth(const RGBA_pixel *const src, const RGBA_pixel *const prev, unsigned char *dest, const uint32_t width)
{
    // Filt(x) = (Orig(x) - PaethPredictor(Orig(l), Orig(u), Orig(ul))) % 256
    dest[0] = PNG_FILTER_PAETH; // Set filter to Paeth

    // Set most left byte (the element before start of row is 0)
    dest[1] = (((unsigned char *)src)[0] - paeth_predictor(0, ((unsigned char *)prev)[0], 0)) % 256;
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    for (size_t i = 1; i < temp_len; i++)
    {
        // + 1 in dest is for skipping the filter byte
        dest[i + 1] = (((unsigned char *)src)[i] -
                       paeth_predictor(((unsigned char *)src)[i - 1], ((unsigned char *)prev)[i], ((unsigned char *)prev)[i - 1])) %
                      256;
    }
}

// Find filter type functions

static inline unsigned long int sum_row_for_heuristics(unsigned char *temp_buffer, const uint32_t width)
{
    unsigned long int sum = 0;
    register size_t temp_len = width * RGBA_PIXEL_SIZE;

    // Sub 256 if element >= 128
    for (size_t i = 1; i <= temp_len; i++)
    {
        if (temp_buffer[i] >= 128)
        {
            temp_buffer[i] = 256 - ((int)temp_buffer[i]);
        }

        // Sum
        sum += temp_buffer[i];
    }

    return sum;
}

static inline int find_min(const unsigned long int arr[], const uint32_t length)
{
    unsigned long int min = arr[0];
    int index = 0;

    for (size_t i = 1; i < length; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
            index = i;
        }
    }

    return index;
}

static unsigned char calc_filter_type(const RGBA_pixel *const src, const RGBA_pixel *const prev, const uint32_t width)
{
    // Allocate storage for temporary row with filter
    unsigned char *temp_buffer = (unsigned char *)malloc(width * RGBA_PIXEL_SIZE + 1);
    unsigned long int sum[PNG_FILTER_COUNT] = {0};

    if (temp_buffer == NULL)
    {
        return PNG_FILTER_COUNT; // Error
    }

    // Call filter None for heuristics
    apply_heur_png_filter_none(src, temp_buffer, width);

    sum[PNG_FILTER_NONE] = sum_row_for_heuristics(temp_buffer, width);

    // Call filter Sub for heuristics
    apply_heur_png_filter_sub(src, temp_buffer, width);

    sum[PNG_FILTER_SUB] = sum_row_for_heuristics(temp_buffer, width);

    // Call filter Up for heuristics
    apply_heur_png_filter_up(src, prev, temp_buffer, width);

    sum[PNG_FILTER_UP] = sum_row_for_heuristics(temp_buffer, width);

    // Call filter Average for heuristics
    apply_heur_png_filter_average(src, prev, temp_buffer, width);

    sum[PNG_FILTER_AVERAGE] = sum_row_for_heuristics(temp_buffer, width);

    // Call filter Paeth for heuristics
    apply_heur_png_filter_paeth(src, prev, temp_buffer, width);

    sum[PNG_FILTER_PAETH] = sum_row_for_heuristics(temp_buffer, width);

    // Free temporary memory
    free(temp_buffer);

    // Find index of min sum
    return find_min(sum, PNG_FILTER_COUNT);
}

// Other

static RGBA_pixel *
strip_filter_per_rgba_row(const unsigned char *filtered_row, const RGBA_pixel *previous_row, const uint32_t width)
{
    // Allocate storage for one row
    RGBA_pixel *result = (RGBA_pixel *)malloc(width * RGBA_PIXEL_SIZE);

    if (result == NULL)
    {
        return NULL;
    }

    // Choose defiltration for current row
    switch (filtered_row[0])
    {
    case PNG_FILTER_NONE:
        reverse_rgba_png_filter_none(filtered_row, result, width);
        break;

    case PNG_FILTER_SUB:
        reverse_rgba_png_filter_sub(filtered_row, result, width);
        break;

    case PNG_FILTER_UP:
        reverse_rgba_png_filter_up(filtered_row, previous_row, result, width);
        break;

    case PNG_FILTER_AVERAGE:
        reverse_rgba_png_filter_average(filtered_row, previous_row, result, width);
        break;

    case PNG_FILTER_PAETH:
        reverse_rgba_png_filter_paeth(filtered_row, previous_row, result, width);
        break;

    default:
        return NULL;
        break;
    }

    return result;
}

// Header defined functions

unsigned char *filter_rgba_png(const IHDR_chunk ihdr, RGBA_pixel **unfiltered_image, unsigned long int *const length)
{
    // Calculate length for filtered data buffer. Size of image * Pixels in RGBA format + filter type markers
    *length = ihdr.m_height * ihdr.m_width * RGBA_PIXEL_SIZE * sizeof(unsigned char) + ihdr.m_height;

    // Allocate filtered buffer
    unsigned char *result = (unsigned char *)malloc(*length);

    if (result == NULL)
    {
        return NULL;
    }

    // Unfilter and append every row to unfiltered image
    RGBA_pixel *temp_row = (RGBA_pixel *)malloc(ihdr.m_width * RGBA_PIXEL_SIZE);

    if (temp_row == NULL)
    {
        return NULL;
    }

    // The row before first is 0 by specifiaction
    memset(temp_row, 0, ihdr.m_width * RGBA_PIXEL_SIZE);

    // Filter first row
    switch (calc_filter_type(unfiltered_image[0], temp_row, ihdr.m_width))
    {
    case PNG_FILTER_NONE:
        apply_rgba_png_filter_none(unfiltered_image[0], result, ihdr.m_width);
        break;

    case PNG_FILTER_SUB:
        apply_rgba_png_filter_sub(unfiltered_image[0], result, ihdr.m_width);
        break;

    case PNG_FILTER_UP:
        apply_rgba_png_filter_up(unfiltered_image[0], temp_row, result, ihdr.m_width);
        break;

    case PNG_FILTER_AVERAGE:
        apply_rgba_png_filter_average(unfiltered_image[0], temp_row, result, ihdr.m_width);
        break;

    case PNG_FILTER_PAETH:
        apply_rgba_png_filter_paeth(unfiltered_image[0], temp_row, result, ihdr.m_width);
        break;

    default:
        return NULL;
        break;
    }

    // Free memory
    free(temp_row);

    // Filter the rest of the rows
    for (size_t i = 1; i < ihdr.m_height; i++)
    {
        switch (calc_filter_type(unfiltered_image[i], unfiltered_image[i - 1], ihdr.m_width))
        {
        case PNG_FILTER_NONE:
            apply_rgba_png_filter_none(unfiltered_image[i],
                                       &result[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)],
                                       ihdr.m_width);
            break;

        case PNG_FILTER_SUB:
            apply_rgba_png_filter_sub(unfiltered_image[i],
                                      &result[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)],
                                      ihdr.m_width);
            break;

        case PNG_FILTER_UP:
            apply_rgba_png_filter_up(unfiltered_image[i],
                                     unfiltered_image[i - 1],
                                     &result[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)],
                                     ihdr.m_width);
            break;

        case PNG_FILTER_AVERAGE:
            apply_rgba_png_filter_average(unfiltered_image[i],
                                          unfiltered_image[i - 1],
                                          &result[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)],
                                          ihdr.m_width);
            break;

        case PNG_FILTER_PAETH:
            apply_rgba_png_filter_paeth(unfiltered_image[i],
                                        unfiltered_image[i - 1],
                                        &result[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)],
                                        ihdr.m_width);
            break;

        default:
            return NULL;
            break;
        }
    }

    return result;
}

RGBA_pixel **unfilter_rgba_png(const unsigned char *const filtered_buffer, IHDR_chunk ihdr)
{
    // Allocate height
    RGBA_pixel **result = (RGBA_pixel **)malloc(ihdr.m_height * sizeof(RGBA_pixel *));

    if (result == NULL)
    {
        return NULL;
    }

    // Unfilter and append every row to unfiltered image
    RGBA_pixel *temp_row = (RGBA_pixel *)malloc(ihdr.m_width * RGBA_PIXEL_SIZE);

    if (temp_row == NULL)
    {
        return NULL;
    }

    // The row before first is 0 by specifiaction
    memset(temp_row, 0, ihdr.m_width * RGBA_PIXEL_SIZE);

    // Unfilter first row
    result[0] = strip_filter_per_rgba_row(&filtered_buffer[0], temp_row, ihdr.m_width);

    // Free memory
    free(temp_row);

    // Unfilter the rest of the rows
    for (size_t i = 1; i < ihdr.m_height; i++)
    {
        result[i] = strip_filter_per_rgba_row(&filtered_buffer[i * (ihdr.m_width * RGBA_PIXEL_SIZE + 1)], result[i - 1], ihdr.m_width);
    }

    return result;
}
