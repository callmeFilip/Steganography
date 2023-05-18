#ifndef PROGRAM_INPUT_PARSER_H
#define PROGRAM_INPUT_PARSER_H
#include <stdbool.h>

// Boundaries
#define PROGRAM_INPUT_PARSER_MAX_INPUT_LENGTH 255

// Error codes
#define PROGRAM_INPUT_PARSER_OK 0
#define PROGRAM_INPUT_PARSER_ERR_CODE_WRONG_INPUT 1
#define PROGRAM_INPUT_PARSER_ERR_CODE_NO_INPUT 2

/**
 * @brief Struct used for ecapsulating program input
 *
 * m_encode is true for encode operation,
 * false for decode operation.
 * Defaults to decode
 */
typedef struct
{
    const char *m_input_name;
    const char *m_output_name;
    bool m_encode;
    const char *m_operation_argument;
    int m_error_code;
} program_inp;

/**
 * @brief Parse and validate input from program
 *
 * @param argc Argument count
 * @param argv Argument array pointer
 * @return program_inp
 */
program_inp parse_program_input(int argc, char const *argv[]);

#endif // ~PROGRAM_INPUT_PARSER_H
