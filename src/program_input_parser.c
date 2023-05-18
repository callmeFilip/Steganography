#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../inc/program_input_parser.h"
#include "../inc/global_config.h"

// Flags
#define FLAG_IDENTIFICATOR "-"
#define FLAG_IDENTIFICATOR_LENGTH 1

#define FLAG_HELP FLAG_IDENTIFICATOR "help"
#define FLAG_INPUT_FILE FLAG_IDENTIFICATOR "i"
#define FLAG_OUTPUT_FILE FLAG_IDENTIFICATOR "o"
#define FLAG_ENCODE FLAG_IDENTIFICATOR "e"
#define FLAG_DECODE FLAG_IDENTIFICATOR "d"

#define FLAG_ARGUMENT_MIN_LENGTH 1

// Files constants
#define TXT_PNG_EXTENSION_LENGTH 4 // .png and .txt are both 4 long
#define EXTENSION_PNG ".png"
#define EXTENSION_TXT ".txt"

static inline void print_help_menu(char const *program_name)
{
    printf("Usage: %s " FLAG_INPUT_FILE " <input_image> [usage_option]\n\n"
           "Where usage options are:\n\n"
           "\t" FLAG_ENCODE " <string>\n"
           "\t\tencode <string> in <input_image> and save it in <input_image>.png\n\n"
           "\t" FLAG_DECODE " <output_file_name>\n"
           "\t\tdecode string from <input_image> and output it in <output_file_name>.txt; "
           "defaults to: <input_image>.txt\n\n"
           "\t" FLAG_OUTPUT_FILE " <output_dir>\n"
           "\t\tset output directory to <output_dir>\n\n\n\n"
           "*note: If no usage options are used, the program defaults to decode mode;\n"
           "       output file name is default and output is produced in current directory!\n",
           program_name);
}

program_inp parse_program_input(int argc, char const *argv[])
{
    program_inp result = {"", "", false, "", 0};
    int valid_args_found = 1;     // starts from 1 because 0 is name of program
    bool is_input_set = false;    // m_input_name
    bool is_output_set = false;   // m_output_name
    bool is_encoding_set = false; // m_encode

    char *output_name_buff = NULL;
    int image_name_len = 0;

    // Check for "-help" usage
    if (argc == 2)
    {
        if (strcmp(argv[1], FLAG_HELP) == 0)
        {
            result.m_error_code = PROGRAM_INPUT_PARSER_ERR_CODE_NO_INPUT;
            print_help_menu(argv[0]);

            return result;
        }
    }

    // Parse for normal usage flags
    for (int i = 1; i < argc; i += 2)
    {
        // Parse input flag
        if (strcmp(FLAG_INPUT_FILE, argv[i]) == 0 && i + 1 < argc && FLAG_ARGUMENT_MIN_LENGTH < strlen(argv[i + 1]) &&
            strncmp(FLAG_IDENTIFICATOR, argv[i + 1], FLAG_IDENTIFICATOR_LENGTH))
        {
            if (is_input_set == false)
            {
                is_input_set = true;

                // Check for input overflow
                if (strlen(argv[i + 1]) > PROGRAM_INPUT_PARSER_MAX_INPUT_LENGTH)
                {
                    break;
                }

                valid_args_found += 2;

                result.m_input_name = argv[i + 1];
            }
        }
        // Parse output dir flag
        else if (strcmp(FLAG_OUTPUT_FILE, argv[i]) == 0 && i + 1 < argc && FLAG_ARGUMENT_MIN_LENGTH < strlen(argv[i + 1]) &&
                 strncmp(FLAG_IDENTIFICATOR, argv[i + 1], FLAG_IDENTIFICATOR_LENGTH))
        {
            if (is_output_set == false)
            {
                is_output_set = true;

                // Check for input overflow
                if (strlen(argv[i + 1]) > PROGRAM_INPUT_PARSER_MAX_INPUT_LENGTH)
                {
                    break;
                }

                valid_args_found += 2;

                result.m_output_name = argv[i + 1];
            }
        }
        // Parse encode flag
        else if (strcmp(FLAG_ENCODE, argv[i]) == 0 && i + 1 < argc && FLAG_ARGUMENT_MIN_LENGTH < strlen(argv[i + 1]) &&
                 strncmp(FLAG_IDENTIFICATOR, argv[i + 1], FLAG_IDENTIFICATOR_LENGTH))
        {
            if (is_encoding_set == false)
            {
                is_encoding_set = true;

                // Check for input overflow
                if (strlen(argv[i + 1]) > PROGRAM_INPUT_PARSER_MAX_INPUT_LENGTH)
                {
                    break;
                }

                valid_args_found += 2;

                result.m_encode = true;
                result.m_operation_argument = argv[i + 1];
            }
        }
        // Parse decode flag
        else if (strcmp(FLAG_DECODE, argv[i]) == 0 && i + 1 < argc && FLAG_ARGUMENT_MIN_LENGTH < strlen(argv[i + 1]) &&
                 strncmp(FLAG_IDENTIFICATOR, argv[i + 1], FLAG_IDENTIFICATOR_LENGTH))
        {
            if (is_encoding_set == false)
            {
                is_encoding_set = true;

                valid_args_found += 2;

                result.m_encode = false;
                result.m_operation_argument = argv[i + 1];
            }
        }
    }

    // Verification
    if (!is_input_set)
    {
        result.m_error_code = PROGRAM_INPUT_PARSER_ERR_CODE_NO_INPUT;
        printf("[Error] Invalid usage of program! Try: %s -help\n", argv[0]);

        return result;
    }
    else if (valid_args_found != argc)
    {
        result.m_error_code = PROGRAM_INPUT_PARSER_ERR_CODE_WRONG_INPUT;
        printf("[Error] Invalid usage of program! Try: %s -help\n", argv[0]);

        return result;
    }

    // Parse output path
    if (strlen(result.m_operation_argument) == 0 && result.m_encode == false)
    {

        // If output file name is not set and program is in decoding mode => copy name
        // Find output name length
        for (int i = strlen(result.m_input_name) - 1; i >= 0; i--)
        {
            if (result.m_input_name[i] == OS_PATH_SEPARATOR)
            {
                break;
            }

            image_name_len++;
        }

        image_name_len -= TXT_PNG_EXTENSION_LENGTH - 1; // - (-1) for null termination character

        // Allocate and initialize storage for output name string
        output_name_buff = (char *)malloc(image_name_len * sizeof(char) + TXT_PNG_EXTENSION_LENGTH);

        if (output_name_buff == NULL)
        {
            result.m_error_code = PROGRAM_INPUT_PARSER_ERR_CODE_WRONG_INPUT;
            printf("[Error] Invalid usage of program! Try: %s -help\n", argv[0]);

            return result;
        }

        // Copy image name from input into buffer
        strncpy(output_name_buff,
                result.m_input_name + strlen(result.m_input_name) * sizeof(char) - image_name_len + 1 - TXT_PNG_EXTENSION_LENGTH,
                image_name_len * sizeof(char) - 1);

        if (result.m_encode == true)
        {
            // If encoding => Output will be name.png
            strncpy(output_name_buff + image_name_len * sizeof(char) - 1, EXTENSION_PNG, TXT_PNG_EXTENSION_LENGTH + 1); // + 1 for NULL character
        }
        else
        {
            // If decoding => Output will be name.txt
            strncpy(output_name_buff + image_name_len * sizeof(char) - 1, EXTENSION_TXT, TXT_PNG_EXTENSION_LENGTH + 1); // + 1 for NULL character
        }

        // Append file name to output directory
        {
            char *temp_storage = malloc(strlen(result.m_output_name) + strlen(output_name_buff) + 1); // + 1 for NULL character
            sprintf(temp_storage, "%s%s", result.m_output_name, output_name_buff);

            result.m_output_name = temp_storage;
        }

        // Clean up buffer
        free(output_name_buff);
    }
    // If output file name is set and is in decoding mode => append name from argument
    else if (strlen(result.m_operation_argument) > 0 && result.m_encode == false)
    {
        char *temp_storage = malloc(strlen(result.m_output_name) + strlen(result.m_operation_argument) + 1); // + 1 for NULL character
        sprintf(temp_storage, "%s%s", result.m_output_name, result.m_operation_argument);

        result.m_output_name = temp_storage;
    }
    // If program is in decoding mode => copy name
    else if (result.m_encode == true)
    {
        // Find output name length
        for (int i = strlen(result.m_input_name) - 1; i >= 0; i--)
        {
            if (result.m_input_name[i] == OS_PATH_SEPARATOR)
            {
                break;
            }

            image_name_len++;
        }

        image_name_len -= TXT_PNG_EXTENSION_LENGTH - 1; // - (-1) for null termination character

        // Allocate and initialize storage for output name string
        output_name_buff = (char *)malloc(image_name_len * sizeof(char) + TXT_PNG_EXTENSION_LENGTH);

        if (output_name_buff == NULL)
        {
            result.m_error_code = PROGRAM_INPUT_PARSER_ERR_CODE_WRONG_INPUT;
            printf("[Error] Invalid usage of program! Try: %s -help\n", argv[0]);

            return result;
        }

        // Copy image name from input into buffer
        strncpy(output_name_buff,
                result.m_input_name + strlen(result.m_input_name) * sizeof(char) - image_name_len + 1 - TXT_PNG_EXTENSION_LENGTH,
                image_name_len * sizeof(char) - 1);

        if (result.m_encode == true)
        {
            // If encoding => Output will be name.png
            strncpy(output_name_buff + image_name_len * sizeof(char) - 1, EXTENSION_PNG, TXT_PNG_EXTENSION_LENGTH + 1); // + 1 for NULL character
        }
        else
        {
            // If decoding => Output will be name.txt
            strncpy(output_name_buff + image_name_len * sizeof(char) - 1, EXTENSION_TXT, TXT_PNG_EXTENSION_LENGTH + 1); // + 1 for NULL character
        }

        // Append file name to output directory
        {
            char *temp_storage = malloc(strlen(result.m_output_name) + strlen(output_name_buff) + 1); // + 1 for NULL character
            sprintf(temp_storage, "%s%s", result.m_output_name, output_name_buff);

            result.m_output_name = temp_storage;
        }

        // Clean up buffer
        free(output_name_buff);
    }

    return result;
}
