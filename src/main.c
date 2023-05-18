#include "../inc/global_config.h"
#include "../inc/program_input_parser.h"
#include "../inc/codec.h"

int main(int argc, char const *argv[])
{
    program_inp input = parse_program_input(argc, argv);

    if (input.m_error_code != PROGRAM_INPUT_PARSER_OK)
    {
        return input.m_error_code;
    }

    if (input.m_encode == true)
    {
        return encoding(input);
    }
    else
    {
        return decoding(input);
    }

    return PROGRAM_OK;
}
