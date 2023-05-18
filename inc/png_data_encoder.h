#ifndef PNG_DATA_ENCODER
#define PNG_DATA_ENCODER

#include "../inc/png_filtration.h"
#include "stdbool.h"

/**
 * @brief Encode data with length data_length in image
 *
 * @param image Image to encode data in
 * @param ihdr Header of the image that is being used for encoding
 * @param data Buffer with data that will be encoded
 * @param data_length Returns length of data buffer
 * @return True if successful, false if not
 */
bool encode_data_rgba(RGBA_pixel **image, const IHDR_chunk ihdr,
                      unsigned char *const data, uint32_t data_length);

/**
 * @brief Decode data from image
 *
 * @param image Image to decode data from
 * @param ihdr Header of the image that is being used
 * @param data Buffer with data that will be filled
 * @param data_length Returns length of data buffer
 * @return True if successful, false if not
 */
bool decode_data_rgba(RGBA_pixel **const image, const IHDR_chunk ihdr,
                      unsigned char **data, uint32_t *data_length);

#endif // ~PNG_DATA_ENCODER_H
