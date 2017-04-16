/**
  * @brief      Embedded Linux TPM Toolbox 2 (ELTT2)
  * @details    eltt2.c implements some basic methods to communicate with the Infineon TPM 2.0 without the TDDL lib.
  * @file       eltt2.c
  * @date       2014/06/26
  * @copyright  Copyright (c) 2014, Infineon Technologies AG\n
  * All rights reserved.\n
  * \n
  * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
  * conditions are met:\n
  * \n
  * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
  * disclaimer.\n
  * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
  * disclaimer in the documentation and/or other materials provided with the distribution.\n
  * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
  * derived from this software without specific prior written permission.\n
  * \n
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#include "eltt2.h"

/**
  * @brief      Main entry point of the application.
  * @details    Handles the command line input and starts the communication with the TPM.
  * @param      [in]    argc                    Counter for input parameters.
  * @param      [in]    **argv                  Input parameters.
  * @return     One of the listed return codes, the TPM return code or the error code stored in the global errno system variable.
  * @retval     EXIT_SUCCESS                    In case of success.
  * @retval     ERR_BAD_CMD                     In case an invalid command line option.
  * @retval     value of errno                  In case of memory allocation error.
  * @retval     tpmtool_transmit                All error codes from tpmtool_transmit.
  * @retval     return_error_handling           All error codes from return_error_handling.
  * @retval     response_print                  All error codes from response_print.
  * @retval     create_hash_sequence            All error codes from create_hash_sequence.
  * @retval     hexstr_to_bytearray             All error codes from hexstr_to_bytearray.
  * @retval     pcr_extend                      All error codes from pcr_extend.
  * @retval     get_random                      All error codes from get_random.
  * @retval     pcr_read                        All error codes from pcr_read.
  * @retval     create_hash                     All error codes from create_hash.
  * @retval     pcr_reset                       All error codes from pcr_reset.
  * @date       2014/06/26
  */
int main(int argc, char **argv)
{
        // ---------- Local declarations ----------
        int ret_val = EXIT_SUCCESS;        // Return value.
        uint8_t *tpm_response_buf = NULL;  // Buffer for TPM response.
        ssize_t tpm_response_buf_size = 0; // Size of tpm_response_buf.
        int i = 0;                         // Command line parsing counter.
        int option = 0;                    // Command line option.
        uint8_t *input_bytes = NULL;       // Custom command bytes for transmit in case of command line options -b and -E.
        size_t input_bytes_size = 0;       // Size of input_bytes.
        int no_transmission = 0;           // Flag to skip the transmission call, e.g. in case of command line option -h.
        int tpm_error = 0;                 // Flag to indicate whether a TPM response has returned a TPM error code or not.

        // ---------- Program flow ----------
        printf("\n");
        do // Begin of DO WHILE(FALSE) for error handling.
        {
                // ---------- Allocate memory for buffer containing TPM response ----------
                tpm_response_buf_size = TPM_RESP_MAX_SIZE;
                tpm_response_buf = malloc(tpm_response_buf_size);
                MALLOC_ERROR_CHECK(tpm_response_buf);
                memset(tpm_response_buf, 0xFF, tpm_response_buf_size);

                // ---------- Check for command line parameters ----------
                if (1 == argc)
                {
                        fprintf(stderr, "ELTT needs an option. Use '-h' for displaying help.\n");
                        ret_val = ERR_BAD_CMD;
                        break;
                }

                // ---------- Command line parsing with getopt ----------
                opterr = 0; // Disable getopt error messages in case of unknown parameters; we want to use our own error messages.

                // Loop through parameters with getopt.
                while (-1 != (option = getopt(argc, argv, "cgvhTa:A:b:d:e:E:G:r:R:s:S:t:u:z:")))
                {
                        switch (option)
                        {
                                case 'a': // TPM2_HashSequenceStart SHA-1
                                case 'A': // TPM2_HashSequenceStart SHA-256
                                        ret_val = create_hash_sequence(optarg, option, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'b': // Enter your own command bytes
                                        // Allocate the input buffer for hexstr_to_bytearray and tpmtool_transmit.
                                        input_bytes_size = strlen(optarg) / 2 + strlen(optarg) % 2; // 2 characters == 1 byte => size of input command bytes: length of input string / 2.
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0xFF, input_bytes_size);

                                        // Convert the command line input to bytes.
                                        ret_val = hexstr_to_bytearray(optarg, input_bytes, input_bytes_size);
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'c': // TPM_CC_ReadClock
                                        ret_val = tpmtool_transmit(tpm_cc_readclock, sizeof(tpm_cc_readclock), tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'd': // TPM_CC_Shutdown
                                        if (0 == strcasecmp(optarg, "clear"))
                                        {
                                                ret_val = tpmtool_transmit(tpm_cc_shutdown_clear, sizeof(tpm_cc_shutdown_clear), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else if (0 == strcasecmp(optarg, "state"))
                                        {
                                                ret_val = tpmtool_transmit(tpm_cc_shutdown_state, sizeof(tpm_cc_shutdown_state), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else
                                        {
                                                ret_val = ERR_BAD_CMD;
                                                fprintf(stderr, "Unknown option. Use '-h' for more information.\n");
                                        }
                                        break;

                                case 'e': // PCR_Extend SHA-1
                                case 'E': // PCR_Extend SHA-256
                                        if (4 > argc)
                                        {
                                                ret_val = ERR_BAD_CMD;
                                                fprintf(stderr, "The command '-%c' needs two arguments. Use '-h' for more information.\n", option);

                                                // Set the argument count to the next option for error handling.
                                                optind += 2;
                                                break;
                                        }

                                        // Allocate the input buffer for pcr_extend and tpmtool_transmit.
                                        if ('e' == option)
                                        {
                                                input_bytes_size = sizeof(tpm2_pcr_extend) + TPM_SHA1_DIGEST_SIZE;
                                        }
                                        else
                                        {
                                                input_bytes_size = sizeof(tpm2_pcr_extend) + TPM_SHA256_DIGEST_SIZE;
                                        }
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0, input_bytes_size);

                                        // Create PCR_Extend TPM request.
                                        ret_val = pcr_extend(optarg, argv[optind], input_bytes, input_bytes_size, option);

                                        // Set the argument count to the next option for error handling.
                                        optind++;
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'g': // TPM_CC_GetCapability
                                        ret_val = tpmtool_transmit(tpm2_getcapability_fixed, sizeof(tpm2_getcapability_fixed), tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'v': // TPM_CC_GetCapability
                                        ret_val = tpmtool_transmit(tpm2_getcapability_var, sizeof(tpm2_getcapability_var), tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'G': // TPM_CC_GetRandom
                                        // Allocate the input buffer for get_random and tpmtool_transmit.
                                        input_bytes_size = (sizeof(tpm2_getrandom));
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0, input_bytes_size);

                                        // Create GetRandom TPM request.
                                        ret_val = get_random(optarg, input_bytes);
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'h': // Help
                                        print_help();

                                        // Set flag to skip any TPM transmission
                                        no_transmission = 1;
                                        break;

                                case 'r': // PCR_Read SHA-1
                                case 'R': // PCR_Read SHA-256
                                        // Allocate the input buffer for pcr_read and tpmtool_transmit.
                                        input_bytes_size = sizeof(tpm2_pcr_read);
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0, input_bytes_size);

                                        // Create PCR_Read TPM request.
                                        ret_val = pcr_read(optarg, input_bytes, option);
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 's': // Hash SHA-1
                                case 'S': // Hash SHA-256
                                        // Allocate the input buffer for create_hash and tpmtool_transmit.
                                        input_bytes_size = strlen(optarg) / 2 + strlen(optarg) % 2 + sizeof(tpm2_hash);
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0, input_bytes_size);

                                        // Create Hash TPM request.
                                        ret_val = create_hash(optarg, option, input_bytes, input_bytes_size);
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 't': // TPM2_SelfTest
                                        if (0 == strcasecmp(optarg, "not_full"))
                                        {
                                                ret_val = tpmtool_transmit(tpm2_self_test, sizeof(tpm2_self_test), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else if (0 == strcasecmp(optarg, "full"))
                                        {
                                                ret_val = tpmtool_transmit(tpm2_self_test_full, sizeof(tpm2_self_test_full), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else if (0 == strcasecmp(optarg, "incremental"))
                                        {
                                                ret_val = tpmtool_transmit(tpm2_self_test_incremental, sizeof(tpm2_self_test_incremental), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else
                                        {
                                                ret_val = ERR_BAD_CMD;
                                                fprintf(stderr, "Unknown option. Use '-h' for more information.\n");
                                        }
                                        break;

                                case 'T': // TPM_CC_GetTestResult
                                        ret_val = tpmtool_transmit(tpm_cc_get_test_result, sizeof(tpm_cc_get_test_result), tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                case 'u': // TPM2_Startup
                                        if (0 == strcasecmp(optarg, "clear"))
                                        {
                                                ret_val = tpmtool_transmit(tpm2_startup_clear, sizeof(tpm2_startup_clear), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else if (0 == strcasecmp(optarg, "state"))
                                        {
                                                ret_val = tpmtool_transmit(tpm2_startup_state, sizeof(tpm2_startup_state), tpm_response_buf, &tpm_response_buf_size);
                                        }
                                        else
                                        {
                                                ret_val = ERR_BAD_CMD;
                                                fprintf(stderr, "Unknown option. Use '-h' for more information.\n");
                                        }
                                        break;

                                case 'z': // PCR_Reset
                                        // Allocate the input buffer for pcr_reset and tpmtool_transmit
                                        input_bytes_size = sizeof(tpm2_pcr_reset);
                                        input_bytes = malloc(input_bytes_size);
                                        MALLOC_ERROR_CHECK(input_bytes);
                                        memset(input_bytes, 0, input_bytes_size);

                                        // Create PCR_Reset TPM request.
                                        ret_val = pcr_reset(optarg, input_bytes);
                                        RET_VAL_CHECK(ret_val);

                                        // Send bytes to TPM.
                                        ret_val = tpmtool_transmit(input_bytes, input_bytes_size, tpm_response_buf, &tpm_response_buf_size);
                                        break;

                                default:
                                        if ('a' == optopt || 'A' == optopt || 'b' == optopt || 'e' == optopt || 'E' == optopt || 'G' == optopt ||
                                            'r' == optopt || 'R' == optopt || 's' == optopt || 'S' == optopt || 'z' == optopt)
                                        {
                                                // Error output if arguments are missing.
                                                fprintf(stderr, "Option '-%c' requires additional arguments. Use '-h' for more information.\n", optopt);
                                                ret_val = ERR_BAD_CMD;
                                        }
                                        else if ('d' == optopt)
                                        {
                                                // TPM shutdown default option without parameter (default is tpm_cc_shutdown_clear).
                                                ret_val = tpmtool_transmit(tpm_cc_shutdown_clear, sizeof(tpm_cc_shutdown_clear), tpm_response_buf, &tpm_response_buf_size);
                                                option='d'; // for response_print handler
                                        }
                                        else if ('t' == optopt)
                                        {
                                                // TPM shutdown default option without parameter (default is tpm2_self_test).
                                                ret_val = tpmtool_transmit(tpm2_self_test, sizeof(tpm2_self_test), tpm_response_buf, &tpm_response_buf_size);
                                                option='t'; // for response_print handler
                                        }
                                        else if ('u' == optopt)
                                        {
                                                // TPM startup default option without parameter (default is tpm2_startup_clear).
                                                ret_val = tpmtool_transmit(tpm2_startup_clear, sizeof(tpm2_startup_clear), tpm_response_buf, &tpm_response_buf_size);
                                                option='u'; // for response_print handler
                                        }
                                        else if (isprint(optopt))
                                        {
                                                // Unknown parameter.
                                                fprintf(stderr, "Unknown option '-%c'. Use '-h' for more information.\n", optopt);
                                                ret_val = ERR_BAD_CMD;
                                        }
                                        else
                                        {
                                                // Non-printable character.
                                                fprintf(stderr, "Invalid command line character. Use '-h' for more information.\n");
                                                ret_val = ERR_BAD_CMD;
                                        }
                                        break;
                        } // End of switch.

                        // ---------- Output and error handling ----------
                        // Check for transmission errors or skipped transmission.
                        if (EXIT_SUCCESS != ret_val || 1 == no_transmission)
                        {
                                // Exit command line parameter parsing loop.
                                break;
                        }

                        // Transmission has been successful, now get TPM return code from TPM response.
                        ret_val = return_error_handling(tpm_response_buf);
                        if (EXIT_SUCCESS != ret_val) // Check for errors
                        {
                                // Set flag to indicate a TPM error.
                                tpm_error = 1;

                                // Go out of command line parameter parsing loop.
                                break;
                        }

                        // Print TPM response
                        ret_val = response_print(tpm_response_buf, tpm_response_buf_size, option);
                        RET_VAL_CHECK(ret_val);

                        // Free memory for next command line option
                        MEMSET_FREE(input_bytes, input_bytes_size);
                } // End of while (cmd line parameter parsing loop).

                // If no error has ocurred so far, handle remaining unknown parameters, if present.
                RET_VAL_CHECK(ret_val); // If we do not check and break here in case of an error, we would override the previous error
                for (i = optind; i < argc; i++)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Non-option argument '%s'. Use '-h' for more information.\n", argv[i]);
                }
        } while (0); // End of DO WHILE FALSE loop.

        // Check for non-TPM error.
        if (EXIT_SUCCESS != ret_val && 1 != tpm_error)
        {
                fprintf(stderr, "Unexpected error: 0x%08X\n", ret_val);
        }

        // Map TPM return value 0x100 (TPM_RC_INITIALIZE) to 0x101 (TPM_RC_FAILURE), since in case you
        // run ELTT 2 in a python script only the lowest byte of the return code is actually being returned.
        // But since the lowest byte of 0x100 is 0x00 (== TPM_RC_SUCCESS), python would not be able to
        // distinguish between 0x000 and 0x100 as return code, therefore we need the mapping.
        if (TPM_RC_INITIALIZE == ret_val)
        {
                ret_val = TPM_RC_FAILURE;
        }

        // ---------- Cleanup ----------
        MEMSET_FREE(tpm_response_buf, tpm_response_buf_size);
        MEMSET_FREE(input_bytes, input_bytes_size);

        printf("\n");
        return ret_val;
}

int tpmtool_transmit(const uint8_t *buf, ssize_t length, uint8_t *response, ssize_t *resp_length)
{
        // ---------- Transmit command given in buf to device with handle given in dev_tpm ----------
        int ret_val = EXIT_SUCCESS; // Return value.
        int dev_tpm = -1;           // TPM device handle.
        ssize_t transmit_size = 0;  // Amount of bytes sent to / received from the TPM.

        do
        {
                // Check input parameters.
                NULL_POINTER_CHECK(buf);
                NULL_POINTER_CHECK(response);
                NULL_POINTER_CHECK(resp_length);

                // ---------- Open TPM device ----------
                dev_tpm = open("/dev/tpm0", O_RDWR);
                if (-1 == dev_tpm)
                {
                        ret_val = errno;
                        perror("Error opening the device");
                        break;
                }

                // Send request data to TPM.
                transmit_size = write(dev_tpm, buf, length);
                if (transmit_size == ERR_COMMUNICATION || length != transmit_size)
                {
                        ret_val = errno;
                        perror("Error sending request to TPM");
                        break;
                }

                // Read the TPM response header.
                transmit_size = read(dev_tpm, response, TPM_RESP_MAX_SIZE);
                if (transmit_size == ERR_COMMUNICATION)
                {
                        ret_val = errno;
                        perror("Error reading response from TPM");
                        break;
                }

                // Update response buffer length with value of data length returned by TPM.
                *resp_length = transmit_size;
        } while (0);

        // ---------- Close TPM device ----------
        if (-1 != dev_tpm)
        {
                // Close file handle.
                close(dev_tpm);

                // Invalidate file handle.
                dev_tpm = -1;
        }

        return ret_val;
}

int return_error_handling(uint8_t *response_buf)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        uint32_t tpm_rc = 0;        // Return code from TPM header.

        do
        {
                NULL_POINTER_CHECK(response_buf);

                ret_val = buf_to_uint64(response_buf, 6, sizeof(tpm_rc), (unsigned long long*)&tpm_rc);
                RET_VAL_CHECK(ret_val);

                // Assign TPM return code to ret_val.
                ret_val = tpm_rc;

                if (TPM_RC_SUCCESS != tpm_rc)
                {
                        fprintf(stderr, "TPM Error 0x%08X: ", tpm_rc);
                }

                // Handle some known TPM return codes.
                switch (tpm_rc)
                {
                        case TPM_RC_SUCCESS: // 0x00
                                break;

                        case TPM_RC_BAD_TAG: // 0x1E
                                fprintf(stderr, "The tag value sent to for a command is invalid.\n");
                                break;

                        case TPM_RC_SIZE: // 0x95
                                fprintf(stderr, "Structure is the wrong size.\n");
                                break;

                        case TPM_RC_INITIALIZE: // 0x100
                                fprintf(stderr, "TPM not initialized.\n");
                                break;

                        case TPM_RC_LOCALITY: // 0x907
                                fprintf(stderr, "Bad locality.\n");
                                break;

                        default:
                                fprintf(stderr, "See TPM Library Specification for more information.\n");
                                break;
                }
        } while (0);

        return ret_val;
}

int response_print(uint8_t *response_buf, size_t resp_size, int option)
{
        int ret_val = EXIT_SUCCESS; // Return value.

        do
        {
                NULL_POINTER_CHECK(response_buf);

                switch (option)
                {
                        case 'a':
                        case 'A': // Print the returned hash number.
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_HASH_WITHOUT_HEADER, PRINT_RESPONSE_HASH);
                                break;
                        case 'b': // Print the response value in hex.
                        case 'e':
                        case 'E': // Print the PCR extend response.
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITH_HEADER, PRINT_RESPONSE_HEADERBLOCKS);
                                break;

                        case 'c': // Print the TPM clock values.
                                ret_val = print_clock_info(response_buf);
                                break;

                        case 'd': // Print "Shutdown works as expected."
                                printf("Shutdown works as expected.\n");
                                break;

                        case 'g': // Print the fixed capability flags.
                                ret_val = print_capability_flags(response_buf, PT_FIXED_SELECTOR);
                                break;

                        case 'v': // Print the variable capability flags.
                                ret_val = print_capability_flags(response_buf, PT_VAR_SELECTOR);
                                break;


                        case 'G': // Print the returned random value in hex.
                                printf("Random value:\n");
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HEX_BLOCK);
                                break;

                        case 'r':
                        case 'R': // Print the response value in hex.
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_PCR_WITHOUT_HEADER, PRINT_RESPONSE_CLEAR);
                                break;

                        case 's':
                        case 'S': // Print the returned hash number.
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HASH);
                                break;

                        case 't': // Print "Successfully tested. Works as expected."
                                printf("Successfully tested. Works as expected.\n");
                                break;

                        case 'T': // Print response value hex without the header.
                                printf("Test Result:               ");
                                ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_CLEAR);
                                break;

                        case 'u': // Print "Startup works as expected."
                                printf("Startup works as expected.\n");
                                break;

                        default: // Print "Done."
                                printf("Done.\n");
                                break;
                }
                printf("\n");
        } while (0);

        return ret_val;
}

int print_response_buf(uint8_t *response_buf, size_t resp_size, uint32_t offset, int format)
{
        int ret_val = EXIT_SUCCESS;       // Return value.
        uint32_t i = 0;                   // Loop variable.
        unsigned long long data_size = 0; // Size of response data.

        do
        {
                NULL_POINTER_CHECK(response_buf);

                switch (format)
                {
                        case PRINT_RESPONSE_CLEAR:
                                for (i = 0; i < resp_size - offset; i++)
                                {
                                        printf("%02X ", response_buf[i + offset]);
                                }
                                break;

                        case PRINT_RESPONSE_HEADERBLOCKS:
                                if (TPM_CMD_HEADER_SIZE > resp_size)
                                {
                                        ret_val = EINVAL;
                                        fprintf(stderr, "Response size is too small.\n");
                                        break;
                                }

                                printf("TPM Response:\n");
                                for (i = 0; i < resp_size - offset; i++)
                                {
                                        printf("%02X ", response_buf[i + offset]);
                                        if (i == 1) // Bytes 0 and 1 are TPM TAG.
                                        {
                                                printf("                        TPM TAG\n");
                                        }
                                        else if (i == 5) // Bytes 2 to 5 are the response length.
                                        {
                                                printf("                  RESPONSE SIZE\n");
                                        }
                                        else if (i == 9) // Last 4 bytes in header are the TPM return code.
                                        {
                                                printf("                  RETURN CODE\n");
                                                if (i + 1 < resp_size - offset)
                                                {
                                                        printf(" Command-specific response Data:\n");
                                                }
                                        }
                                        else if (i >= 10 && (i+1) % 10 == 0) // After all 10 header bytes have been printed, start new line after every 10 bytes of data.
                                        {
                                                printf("\n");
                                        }
                                }
                                break;

                        case PRINT_RESPONSE_HEX_BLOCK:
                                for (i = 0; i < resp_size - offset; i++)
                                {
                                        if (i % 16 == 0)
                                        {
                                                printf("\n0x%08X:   ", i);
                                        }
                                        printf("0x%02X  ", response_buf[i + offset]);
                                }
                                break;

                        case PRINT_RESPONSE_HASH:
                                ret_val = buf_to_uint64(response_buf, offset - 2, 2, &data_size); // Data size always should be stored right before the actual data
                                if (data_size > resp_size - offset)
                                {
                                        ret_val = EINVAL;
                                        fprintf(stderr, "Invalid response data size.\n");
                                        break;
                                }
                                for (i = 0; i < data_size; i++)
                                {
                                        if (i % 8 == 0)
                                        {
                                                printf("\n0x%08X:   ", i);
                                        }
                                        printf("%02X  ", response_buf[i + offset]);
                                }
                                break;

                        default:
                                ret_val = EINVAL;
                                fprintf(stderr, "Unknown output format.\n");
                                break;
                }
        } while (0);

        return ret_val;
}

void print_help()
{
        printf("'-a <data bytes>': Hash Sequence SHA1\n");
        printf("        -> Data bytes: Enter a byte sequence like '0F56...' for {0x0f, 0x56, ...}\n");
        printf("'-A <data bytes>': Hash Sequence SHA256\n");
        printf("        -> Data bytes: Enter a byte sequence like '0F56...' for {0x0f, 0x56, ...}\n");
        printf("'-b <command bytes>': Enter your own TPM command\n");
        printf("        -> Command bytes: Enter your command bytes in hex like '0f56...' for {0x0f, 0x56, ...}\n");
        printf("'-c': Read Clock\n");
        printf("'-d <shutdown type>': Shutdown\n");
        printf("        -> Shutdown types: clear [default], state\n");
        printf("'-e <PCR index> <PCR digest>': PCR Extend SHA1\n");
        printf("        -> PCR index:  Enter the PCR index in hex like '17' for 0x17\n");
        printf("           PCR digest: Enter the value to extend the PCR with in hex like '0f56...' for {0x0f, 0x56, ...}\n");
        printf("'-E <PCR index> <PCR digest>': PCR Extend SHA256\n");
        printf("        -> PCR index:  Enter the PCR index in hex like '17' for 0x17\n");
        printf("           PCR digest: Enter the value to extend the PCR with in hex like '0f56...' for {0x0f, 0x56, ...}\n");
        printf("'-g': Get Capability\n");
        printf("'-G <byte count>': Get Random\n");
        printf("        -> Enter desired number of random bytes in hex like '32' for 0x32\n");
        printf("'-h': Help\n");
        printf("'-r <PCR index>': PCR Read SHA1\n");
        printf("        -> PCR index: Enter PCR number in hex like '17' for 0x17\n");
        printf("'-R <PCR index>': PCR Read SHA256\n");
        printf("        -> PCR index: Enter PCR number in hex like '17' for 0x17\n");
        printf("'-s <data bytes>': Hash SHA1\n");
        printf("        -> Data bytes: Enter a byte sequence like '0F56...' for {0x0f, 0x56, ...}\n");
        printf("'-S <data bytes>': Hash SHA256\n");
        printf("        -> Data bytes: Enter a byte sequence like '0F56...' for {0x0f, 0x56, ...}\n");
        printf("'-t <selftest type>': SelfTest\n");
        printf("        -> Selftest type: not_full [default], full, incremental\n");
        printf("'-T': Get Test Result\n");
        printf("'-u <startup type>': Startup\n");
        printf("        -> Startup types: clear [default], state\n");
        printf("'-z <PCR index>': PCR Reset\n");
        printf("        -> PCR index: Enter PCR number in hex like '17' for 0x17\n");
}

int print_capability_flags(uint8_t *response_buf, uint8_t cap_selector)
{
        int ret_val = EXIT_SUCCESS;           // Return value.
        unsigned long long propertyValue = 0; // Value of the read property.
        unsigned long long i = 0, j = 0;      // Position counter.
        int tmp = 0;                          // Temporary buffer.

        if(cap_selector == PT_FIXED_SELECTOR)
                {
                        do
                                {
                                        NULL_POINTER_CHECK(response_buf);

                                        printf("\nTPM capability information of fixed properties:\n");
                                        printf("=========================================================\n");
                                        printf("TPM_PT_FAMILY_INDICATOR:        %c%c%c%c\n", response_buf[23], response_buf[24], response_buf[25], response_buf[26]);

                                        ret_val = buf_to_uint64(response_buf, 31, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_LEVEL:                   %llu\n", propertyValue);

                                        ret_val = buf_to_uint64(response_buf, 39, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_REVISION:                %llu\n", propertyValue);

                                        ret_val = buf_to_uint64(response_buf, 47, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_DAY_OF_YEAR:             %llu\n", propertyValue);

                                        ret_val = buf_to_uint64(response_buf, 55, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_YEAR:                    %llu\n", propertyValue);

                                        printf("TPM_PT_MANUFACTURER:            %c%c%c%c\n", response_buf[63], response_buf[64], response_buf[65], response_buf[66]);

                                        printf("TPM_PT_VENDOR_STRING:           ");
                                        printf("%c%c%c%c", response_buf[71], response_buf[72], response_buf[73], response_buf[74]);
                                        printf("%c%c%c%c", response_buf[79], response_buf[80], response_buf[81], response_buf[82]);
                                        printf("%c%c%c%c", response_buf[87], response_buf[88], response_buf[89], response_buf[90]);
                                        printf("%c%c%c%c\n", response_buf[95], response_buf[96], response_buf[97], response_buf[98]);

                                        ret_val = buf_to_uint64(response_buf, 103, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_VENDOR_TPM_TYPE:         %llu\n", propertyValue);

                                        ret_val = buf_to_uint64(response_buf, 111, 2, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("TPM_PT_FIRMWARE_VERSION:        %llu", propertyValue);
                                        ret_val = buf_to_uint64(response_buf, 113, 2, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf(".%llu", propertyValue);
                                        ret_val = buf_to_uint64(response_buf, 119, 2, &propertyValue); // Check for output version.
                                        RET_VAL_CHECK(ret_val);

                                        if (2 <= propertyValue) // Print the new FIRMWARE_VERSION output.
                                                {
                                                        ret_val = buf_to_uint64(response_buf, 120, 2, &propertyValue);
                                                        RET_VAL_CHECK(ret_val);

                                                        printf(".%llu", propertyValue);
                                                        ret_val = buf_to_uint64(response_buf, 122, 1, &propertyValue);
                                                        RET_VAL_CHECK(ret_val);

                                                        printf(".%llu\n", propertyValue);

                                                }
                                        else // Print the old FIRMWARE_VERSION output.
                                                {
                                                        ret_val = buf_to_uint64(response_buf, 119, 4, &propertyValue);
                                                        RET_VAL_CHECK(ret_val);

                                                        printf(".%llu\n", propertyValue);
                                                }


                                        printf("\nTPM_PT_MEMORY:\n");
                                        printf("=========================================================\n");

                                        ret_val = buf_to_uint64(response_buf, 207, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        i = 1; // bit 0
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 0 value.
                                        printf("Shared RAM:                     %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i + i; // bit 1
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 1 value.
                                        printf("Shared NV:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 2
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 2 value.
                                        printf("Object Copied To Ram:           %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
                                        //bit 31:3 = reserved

                                        printf("\nTPM_PT_PERMANENT:\n");
                                        printf("=========================================================\n");
                                        ret_val = buf_to_uint64(response_buf, 367, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        i = 1; // bit 0
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 0 value.
                                        printf("Owner Auth Set:                 %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i + i; // bit 1
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 1 value.
                                        printf("Sendorsement Auth Set:          %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 2
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 2 value.
                                        printf("Lockout Auth Set:               %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 3
                                        i = i * 2; // bit 4
                                        i = i * 2; // bit 5
                                        i = i * 2; // bit 6
                                        i = i * 2; // bit 7
                                        //bit 7:3 = reserved

                                        i = i * 2; // bit 8
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 8 value.
                                        printf("Disable Clear:                  %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 9
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 9 value.
                                        printf("In Lockout:                     %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 10
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 10 value.
                                        printf("TPM Generated EPS:              %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
                                        //bit 31:11 = reserved

                                } while (0);
                }
        else if (cap_selector == 2)
                {
                        do
                                {

                                        NULL_POINTER_CHECK(response_buf);

                                        printf("\nTPM capability information of variable properties:\n");
                                        printf("\n\n");

                                        ret_val = buf_to_uint64(response_buf, 31, 4, &propertyValue);
                                        RET_VAL_CHECK(ret_val);

                                        printf("\nTPM_PT_STARTUP_CLEAR:\n");
                                        printf("=========================================================\n");

                                        i = 1; // bit 0
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 0 value.
                                        printf("Ph Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i + i; // bit 1
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 1 value.
                                        printf("Sh Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                        i = i * 2; // bit 2
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 2 value.
                                        printf("Eh Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
                                        //bit 30:3 = reserved
                                        for(j = 2; j < 31; j++) // go to bit 31.
                                                {
                                                        i = i * 2;
                                                }
                                        tmp = ((propertyValue & i) == 0? 0:1); // Check bit 31 value.
                                        printf("Orderly:                        %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

                                } while (0);
                    }

        return ret_val;
}

int print_clock_info(uint8_t *response_buf)
{
        int ret_val = EXIT_SUCCESS;           // Return value.
        unsigned long long propertyValue = 0; // Value of the read property.
        unsigned long long tmp_value = 0;     // Helper variable for calculating actual values.
        unsigned long long sec = 0;           // Value for seconds.
        unsigned long long tmp = 0;           // buf_to_uint64 return value.

        do
        {
                NULL_POINTER_CHECK(response_buf);

                printf("\nClock info:\n");
                printf("=========================================================\n");

                ret_val = buf_to_uint64(response_buf, 10, 8, &propertyValue);
                RET_VAL_CHECK(ret_val);

                printf("Time since the last TPM_Init:\n%llu ms  =  ", propertyValue);

                sec = propertyValue / MILISECOND_TO_SECOND;
                tmp_value = sec / YEAR_SECONDS;
                printf("%llu y, ", tmp_value);

                tmp_value = (sec % YEAR_SECONDS) / DAY_SECONDS;
                printf("%llu d, ", tmp_value);

                tmp_value = ((sec % YEAR_SECONDS) % DAY_SECONDS) / HOUR_SECONDS;
                printf("%llu h, ", tmp_value);

                tmp_value = (((sec % YEAR_SECONDS) % DAY_SECONDS) % HOUR_SECONDS) / MINUTE_SECONDS;
                printf("%llu min, ", tmp_value);

                tmp_value = (((sec % YEAR_SECONDS) % DAY_SECONDS) % HOUR_SECONDS) % MINUTE_SECONDS;
                printf("%llu s, ", tmp_value);

                tmp_value = propertyValue % MILISECOND_TO_SECOND;
                printf("%llu ms\n\n", tmp_value);

                ret_val = buf_to_uint64(response_buf, 18, 8, &propertyValue);
                RET_VAL_CHECK(ret_val);

                printf("Time during which the TPM has been powered:\n%llu ms  =  ", propertyValue);

                sec = propertyValue / MILISECOND_TO_SECOND;
                tmp_value = sec / YEAR_SECONDS;
                printf("%llu y, ", tmp_value);

                tmp_value = (sec % YEAR_SECONDS) / DAY_SECONDS;
                printf("%llu d, ", tmp_value);

                tmp_value = ((sec % YEAR_SECONDS) % DAY_SECONDS) / HOUR_SECONDS;
                printf("%llu h, ", tmp_value);

                tmp_value = (((sec % YEAR_SECONDS) % DAY_SECONDS) % HOUR_SECONDS) / MINUTE_SECONDS;
                printf("%llu min, ", tmp_value);

                tmp_value = (((sec % YEAR_SECONDS) % DAY_SECONDS) % HOUR_SECONDS) % MINUTE_SECONDS;
                printf("%llu s, ", tmp_value);

                tmp_value = propertyValue % MILISECOND_TO_SECOND;
                printf("%llu ms\n\n", tmp_value);

                ret_val = buf_to_uint64(response_buf, 26, 4, &tmp);
                RET_VAL_CHECK(ret_val);

                printf("TPM Reset since the last TPM2_Clear:            %llu\n", tmp);

                ret_val = buf_to_uint64(response_buf, 30, 4, &tmp);
                RET_VAL_CHECK(ret_val);

                printf("Number of times that TPM2_Shutdown:             %llu\n", tmp);

                printf("Safe:                                           %i = %s", response_buf[34], (response_buf[34]? "Yes\n" : "No\n"));
        } while (0);

        return ret_val;
}

int buf_to_uint64(uint8_t *input_buffer, uint32_t offset, uint8_t length, unsigned long long *output_value)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        uint32_t i = 0;             // Loop variable.
        unsigned long long tmp = 0; // Temporary variable for value calculation.

        do
        {
                NULL_POINTER_CHECK(input_buffer);
                NULL_POINTER_CHECK(output_value);

                if (8 >= length)
                {
                        for (i = 0; i < length; i++)
                        {
                                tmp = (tmp << 8) + input_buffer[offset + i];
                        }
                        *output_value = tmp;
                }
                else
                {
                        ret_val = EINVAL;
                        fprintf(stderr, "Bad parameter. Requested conversion amount of %i is to high. The maximum possible amount is 8 bytes.\n", length);
                }
        } while (0);

        return ret_val;
}

int hexstr_to_bytearray(char *byte_string, uint8_t *byte_values, size_t byte_values_size)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        char hex_byte[3] = {0};     // Temporary buffer for input bytes.
        char* invalidChars = NULL;  // Pointer to target buffer where method stores invalid characters.
        uint32_t i = 0;             // Loop variable.

        do
        {
                NULL_POINTER_CHECK(byte_string);
                NULL_POINTER_CHECK(byte_values);

                if (strlen(byte_string) / 2 + strlen(byte_string) % 2 > byte_values_size)
                {
                         ret_val = EINVAL;
                         fprintf(stderr, "Input character string is too long for output buffer.\n");
                         break;
                }

                // Loop "bytewise" through string
                for (i = 0; i < strlen(byte_string); i += 2)
                {
                        // Split input string into "bytes"
                        if (1 == strlen(byte_string + i))
                        {
                                // Assemble a single digit in the hex byte string.
                                hex_byte[0] = byte_string[i];
                                hex_byte[1] = '\0';
                        }
                        else
                        {
                                // Assemble a digit pair in the hex byte string.
                                hex_byte[0] = byte_string[i];
                                hex_byte[1] = byte_string[i + 1];
                                hex_byte[2] = '\0';
                        }

                        // Convert the hex string to an integer.
                        errno = 0;
                        byte_values[i / 2] = strtoul(hex_byte, &invalidChars, 16);
                        if (0 != errno)
                        {
                                ret_val = errno;
                                perror("Error parsing string");
                                break;
                        }
                        if ('\0' != *invalidChars)
                        {
                                ret_val = EINVAL;
                                fprintf(stderr, "Invalid character(s) '%s' while trying to parse '%s' to a byte array.\n", invalidChars, byte_string);
                                break;
                        }
                }
        } while (0);

        return ret_val;
}

int int_to_bytearray(uint64_t input, uint32_t input_size, uint8_t *output_byte)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        uint32_t i;                 // For-while-loop counter.

        do
        {
                NULL_POINTER_CHECK(output_byte);

                for (i = 0; i < input_size; i++)
                {
                        output_byte[i] = input >> ((input_size - 1 - i) * 8);
                }
        } while (0);

        return ret_val;
}

int get_random(char *data_length_string, uint8_t *response_buf)
{
        int ret_val = EXIT_SUCCESS;  // Return value.
        uint8_t bytes_requested = 0; // Amount of random bytes requested by the user.
        size_t byte_string_size = 0; // Size of user input.

        do
        {
                NULL_POINTER_CHECK(data_length_string);
                NULL_POINTER_CHECK(response_buf);

                // Get length of command line input.
                byte_string_size = strlen(data_length_string) / 2 + strlen(data_length_string) % 2;
                if (1 != byte_string_size)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a single hex value (2 characters) without leading '0x'.\n");
                        break;
                }

                // Convert the command line input string for requested random data length to byte.
                ret_val = hexstr_to_bytearray(data_length_string, &bytes_requested, 1);
                RET_VAL_CHECK(ret_val);
                if (32 < bytes_requested || 0 == bytes_requested)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a hex value between 0x01 and 0x20.\n");
                        break;
                }

                // Copy command bytes.
                memcpy(response_buf, tpm2_getrandom, sizeof(tpm2_getrandom));

                // Store amount of requested bytes at the correct byte index in the command byte stream.
                response_buf[sizeof(tpm2_getrandom) - 1] = bytes_requested;
        } while (0);

        return ret_val;
 }

int create_hash(char *data_string, char option, uint8_t *hash_cmd_buf, uint32_t hash_cmd_buf_size)
{
        int ret_val = EXIT_SUCCESS;         // Return value.
        uint32_t offset = 0;                // Helper offset for generating command request.
        uint16_t data_string_size = 0;      // Size of user input data.
        unsigned long max_data_size = 0;    // Maximum supported size for input data.
        const uint8_t *tpm_hash_alg = NULL; // Pointer to hash algorithm identifier.

        do
        {
                NULL_POINTER_CHECK(data_string);
                NULL_POINTER_CHECK(hash_cmd_buf);
                if ('s' != option && 'S' != option)
                {
                        ret_val = EINVAL;
                        fprintf(stderr, "Bad parameter. Option argument must be 's' or 'S'.\n");
                        break;
                }

                // Check input string size.
                data_string_size = strlen(data_string) / 2 + strlen(data_string) % 2;
                max_data_size = TPM_REQ_MAX_SIZE - sizeof(tpm2_hash);
                if (data_string_size > max_data_size)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Maximum input data size is %lu byte (%lu characters).\n", max_data_size, max_data_size * 2);
                        break;
                }

                // Copy basic command bytes.
                memcpy(hash_cmd_buf, tpm2_hash, sizeof(tpm2_hash));

                // Set hash algorithm, command and data sizes depending on user input option at the correct byte index in the command byte stream.
                if ('s' == option)
                {
                        tpm_hash_alg = sha1_alg;
                        printf("\nTPM2_Hash of '%s' with SHA-1:\n", data_string);
                }
                else
                {
                        tpm_hash_alg = sha256_alg;
                        printf("\nTPM2_Hash of '%s' with SHA-256:\n", data_string);
                }

                offset = 2;
                ret_val = int_to_bytearray(sizeof(tpm2_hash) + data_string_size, sizeof(uint32_t), hash_cmd_buf + offset);
                RET_VAL_CHECK(ret_val);
                offset = 10;
                ret_val = int_to_bytearray(data_string_size, sizeof(data_string_size), hash_cmd_buf + offset);
                RET_VAL_CHECK(ret_val);
                offset += sizeof(data_string_size);

                // Copy hash data to TPM request.
                ret_val = hexstr_to_bytearray(data_string, hash_cmd_buf + offset, hash_cmd_buf_size - offset);
                RET_VAL_CHECK(ret_val);
                offset += data_string_size;

                // Set hash algorithm and hierarchy.
                memcpy(hash_cmd_buf + offset, tpm_hash_alg, 2);
                offset += 2;
                memcpy(hash_cmd_buf + offset, tpm_cc_hash_hierarchy, sizeof(tpm_cc_hash_hierarchy));
        } while (0);

        return ret_val;
}

int create_hash_sequence(char *data_string, char option, uint8_t *tpm_response_buf, ssize_t *tpm_response_buf_size)
{
        int ret_val = EXIT_SUCCESS;                 // Return value.
        uint16_t data_string_bytes_size = 0;        // Size of user input data string in bytes.
        uint8_t *data_string_bytes = NULL;          // Buffer for user input data string as bytes.
        uint32_t update_request_size = 0;           // Size of user input string.
        uint16_t transfer_bytes = 0;                // Amount of bytes to be transmitted to the TPM.
        uint16_t remaining_bytes = 0;               // Amount of bytes not yet transmitted to the TPM.
        uint32_t offset = 0;                        // Helper offset for generating command request.
        unsigned long long tpm_rc = TPM_RC_SUCCESS; // TPM return code.
        uint8_t *update_request = NULL;             // Buffer for update sequence command.
        uint8_t sequence_handle[4];                 // Buffer for tpmi response.
		ssize_t original_response_buf_size = 0;     // Backup of the original response buffer size

        do
        {
                NULL_POINTER_CHECK(tpm_response_buf);
                NULL_POINTER_CHECK(data_string);
                NULL_POINTER_CHECK(tpm_response_buf_size);
                if ('a' != option && 'A' != option)
                {
                        ret_val = EINVAL;
                        fprintf(stderr, "Bad parameter. Option argument must be 'a' or 'A'.\n");
                        break;
                }
				original_response_buf_size = *tpm_response_buf_size;

                // Set hash algorithm depending on user input option at the correct byte index in the command byte stream.
                if ('a' == option)
                {
                        printf("\nTPM2_HashSequenceStart of '%s' with SHA-1:\n", data_string);
                        memcpy(tpm2_hash_sequence_start + 12, sha1_alg, sizeof(sha1_alg));
                }
                else
                {
                        printf("\nTPM2_HashSequenceStart of '%s' with SHA-256:\n", data_string);
                        memcpy(tpm2_hash_sequence_start + 12, sha256_alg, sizeof(sha256_alg));
                }

                // Send the TPM2_HashSequenceStart command to the TPM.
                ret_val = tpmtool_transmit(tpm2_hash_sequence_start, sizeof(tpm2_hash_sequence_start), tpm_response_buf, tpm_response_buf_size);
                RET_VAL_CHECK(ret_val);

                // Print the TPM response.
                NULL_POINTER_CHECK(tpm_response_buf_size);
                ret_val = print_response_buf(tpm_response_buf, (size_t)*tpm_response_buf_size, PRINT_RESPONSE_WITH_HEADER, PRINT_RESPONSE_HEADERBLOCKS);
                RET_VAL_CHECK(ret_val);

                // Abort in case of TPM error.
                ret_val = buf_to_uint64(tpm_response_buf, 6, 4, &tpm_rc);
                RET_VAL_CHECK(ret_val);
                if (TPM_RC_SUCCESS != tpm_rc)
                {
                        break;
                }

                // Get sequence handle from TPM response.
                memcpy(sequence_handle, tpm_response_buf + 10, sizeof(sequence_handle));

                // Calculate byte sizes of hash input string.
                data_string_bytes_size = strlen(data_string) / 2 + strlen(data_string) % 2;

                // Allocate memory for conversion of hash input string.
                data_string_bytes = malloc(data_string_bytes_size);
                MALLOC_ERROR_CHECK(data_string_bytes);
                memset(data_string_bytes, 0, data_string_bytes_size);

                // Convert hash input string to bytes.
                ret_val = hexstr_to_bytearray(data_string, data_string_bytes, data_string_bytes_size);
                RET_VAL_CHECK(ret_val);

                // If necessary split input bytes into multiple TPM2_SequenceUpdate commands
                remaining_bytes = data_string_bytes_size;
                while (remaining_bytes > 0)
                {
                        // Calculate amount of bytes to be transmitted next
                        if (remaining_bytes <= TPM_REQ_MAX_SIZE - sizeof(tpm2_sequence_update))
                        {
                                transfer_bytes = remaining_bytes;
                        }
                        else
                        {
                                transfer_bytes = TPM_REQ_MAX_SIZE - sizeof(tpm2_sequence_update);
                        }
                        update_request_size = transfer_bytes + sizeof(tpm2_sequence_update);

                        // Compose the TPM2_SequenceUpdate command.
                        update_request = malloc(update_request_size);
                        MALLOC_ERROR_CHECK(update_request);
                        memset(update_request, 0, update_request_size);
                        memcpy(update_request, tpm2_sequence_update, sizeof(tpm2_sequence_update));
                        offset = 2;
                        ret_val = int_to_bytearray(update_request_size, sizeof(update_request_size), update_request + offset);
                        RET_VAL_CHECK(ret_val);
                        offset = 10;
                        memcpy(update_request + offset, sequence_handle, sizeof(sequence_handle));
                        offset = sizeof(tpm2_sequence_update) - sizeof(transfer_bytes);
                        ret_val = int_to_bytearray(transfer_bytes, sizeof(transfer_bytes), update_request + offset);
                        RET_VAL_CHECK(ret_val);
                        memcpy(update_request + sizeof(tpm2_sequence_update), data_string_bytes + data_string_bytes_size - remaining_bytes, transfer_bytes);

                        // Send the TPM2_SequenceUpdate command to the TPM.
                        printf("\n\nTPM2_SequenceUpdate:\n");
						*tpm_response_buf_size = original_response_buf_size;
                        ret_val = tpmtool_transmit(update_request, update_request_size, tpm_response_buf, tpm_response_buf_size);
                        RET_VAL_CHECK(ret_val);

                        // Free allocated memory
                        MEMSET_FREE(update_request, update_request_size);

                        // Print the TPM response.
                        NULL_POINTER_CHECK(tpm_response_buf_size);
                        ret_val = print_response_buf(tpm_response_buf, (size_t)*tpm_response_buf_size, PRINT_RESPONSE_WITH_HEADER, PRINT_RESPONSE_HEADERBLOCKS);
                        RET_VAL_CHECK(ret_val);

                        // Abort in case of TPM error.
                        ret_val = buf_to_uint64(tpm_response_buf, 6, 4, &tpm_rc);
                        RET_VAL_CHECK(ret_val);
                        if (TPM_RC_SUCCESS != tpm_rc)
                        {
                                break;
                        }

                        remaining_bytes = remaining_bytes - transfer_bytes;
                }
                RET_VAL_CHECK(ret_val);
                if (TPM_RC_SUCCESS != tpm_rc)
                {
                        break;
                }

                // Set the sequence handle in the TPM2_SequenceComplete command.
                memcpy(tpm2_sequence_complete + 10, sequence_handle, sizeof(sequence_handle));

                // Send the TPM2_SequenceComplete command to the TPM.
                printf("\n\nTPM2_SequenceComplete:\n");
				*tpm_response_buf_size = original_response_buf_size;
                ret_val = tpmtool_transmit(tpm2_sequence_complete, sizeof(tpm2_sequence_complete), tpm_response_buf, tpm_response_buf_size);
                RET_VAL_CHECK(ret_val);
				
                // Print the TPM response.
                NULL_POINTER_CHECK(tpm_response_buf_size);
                ret_val = print_response_buf(tpm_response_buf, (size_t)*tpm_response_buf_size, PRINT_RESPONSE_WITH_HEADER, PRINT_RESPONSE_HEADERBLOCKS);
				printf("\n\nHash value extracted from TPM response:\n");
        } while (0);

        MEMSET_FREE(data_string_bytes, data_string_bytes_size);
        MEMSET_FREE(update_request, update_request_size);

        return ret_val;
}

int pcr_extend(char *pcr_index_str, char *pcr_digest_str, uint8_t *pcr_cmd_buf, size_t pcr_cmd_buf_size, char option)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        uint8_t pcr_index = 0;      // PCR index user input byte.
        uint32_t pcr_digest_size = 0; // Sizeof PCR digest user input.

        do
        {
                NULL_POINTER_CHECK(pcr_index_str);
                NULL_POINTER_CHECK(pcr_digest_str);
                NULL_POINTER_CHECK(pcr_cmd_buf);
                if ('e' != option && 'E' != option)
                {
                        ret_val = EINVAL;
                        fprintf(stderr, "Bad parameter. Option argument must be 'e' or 'E'.\n");
                        break;
                }

                // Check and convert the command line input (PCR index) to bytes.
                if (1 != strlen(pcr_index_str) / 2 + strlen(pcr_index_str) % 2)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a single hex value (2 characters) without leading '0x'.\n");
                        break;
                }
                ret_val = hexstr_to_bytearray(pcr_index_str, &pcr_index, 1);
                RET_VAL_CHECK(ret_val);
                if (23 < pcr_index)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a hex value between 0x00 and 0x17 in hex without leading '0x'.\n");
                        break;
                }

                // Check the command line input (PCR digest).
                pcr_digest_size = strlen(pcr_digest_str) / 2 + strlen(pcr_digest_str) % 2;
                if ('e' == option && TPM_SHA1_DIGEST_SIZE < pcr_digest_size)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Maximum SHA-1 PCR digest size is 20 byte (40 characters), but you entered %u byte.\n", pcr_digest_size);
                        break;
                }
                if ('E' == option && TPM_SHA256_DIGEST_SIZE < pcr_digest_size)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Maximum SHA-256 PCR digest size is 32 byte (64 characters), but you entered %u byte.\n", pcr_digest_size);
                        break;
                }

                // Copy basic command bytes.
                memcpy(pcr_cmd_buf, tpm2_pcr_extend, sizeof(tpm2_pcr_extend));

                // Store PCR index at the correct byte index in the command byte stream.
                pcr_cmd_buf[13] = pcr_index;

                // Convert the PCR digest to bytes and store it at the correct byte index in the command byte stream.
                ret_val = hexstr_to_bytearray(pcr_digest_str, pcr_cmd_buf + sizeof(tpm2_pcr_extend), pcr_cmd_buf_size - sizeof(tpm2_pcr_extend));
                RET_VAL_CHECK(ret_val);

                // Set hash algorithm and command length depending on user input option at the correct byte index in the command byte stream.
                if ('e' == option)
                {
                        pcr_cmd_buf[5] = sizeof(tpm2_pcr_extend) + TPM_SHA1_DIGEST_SIZE;
                        memcpy(pcr_cmd_buf + 31, sha1_alg, sizeof(sha1_alg));
                        printf("Extend PCR %i (SHA-1) with digest { ", pcr_index);
                }
                else if ('E' == option)
                {
                        pcr_cmd_buf[5] = sizeof(tpm2_pcr_extend) + TPM_SHA256_DIGEST_SIZE;
                        memcpy(pcr_cmd_buf + 31, sha256_alg, sizeof(sha256_alg));
                        printf("Extend PCR %i (SHA-256) with digest { ", pcr_index);
                }
                print_response_buf(pcr_cmd_buf, pcr_cmd_buf_size, sizeof(tpm2_pcr_extend), PRINT_RESPONSE_CLEAR);
                printf("}:\n");
        } while (0);

        return ret_val;
}

int pcr_read(char *pcr_index_str, uint8_t *pcr_cmd_buf, char option)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        int pcr_byte_index = 0;     // The location for pcr_select on pcr_cmd_buf.
        uint8_t pcr_select = 0;     // PCR index as mapped bit value.
        uint8_t pcr_index = 0;      // PCR user input byte.

        do
        {
                NULL_POINTER_CHECK(pcr_index_str);
                NULL_POINTER_CHECK(pcr_cmd_buf);
                if ('r' != option && 'R' != option)
                {
                        ret_val = EINVAL;
                        fprintf(stderr, "Bad parameter. Option argument must be 'r' or 'R'.\n");
                        break;
                }

                // Convert the command line input to bytes.
                if (1 != strlen(pcr_index_str) / 2 + strlen(pcr_index_str) % 2)
                {
                        // Current implementation only supports selection of one PCR at a time.
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a single hex value (2 characters) without leading '0x'.\n");
                        break;
                }
                ret_val = hexstr_to_bytearray(pcr_index_str, &pcr_index, 1);
                RET_VAL_CHECK(ret_val);
                if (23 < pcr_index)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a hex value between 0x00 and 0x17 in hex without leading '0x'.\n");
                        break;
                }

                // Copy basic command bytes.
                memcpy(pcr_cmd_buf, tpm2_pcr_read, sizeof(tpm2_pcr_read));

                // Calculate the location of PCR index in command byte stream depends on the PCR index value: 0 for PCRs 0-7, 1 for PCRs 8-15 and 2 for PCRs 16-23.
                pcr_byte_index = pcr_index / 8;

                // Set corresponding bit in PCR selection byte to 1: bit 0 for PCRs 0, 8 or 16, bit 1 for PCRs 1, 9 or 17, ... (etc.), and bit 7 for PCRs 7, 15 or 23.
                // Note: This would allow to read multiple PCRs at once. For simplicity reasons, ELTT only supports reading one PCR at a time.
                pcr_select = (1 << (pcr_index % 8));

                // Store pcr_select at the correct byte index in the command byte stream.
                pcr_cmd_buf[17 + pcr_byte_index] = pcr_select;

                // Set hash algorithm depending on user input option at the correct byte index in the command byte stream.
                if ('r' == option)
                {
                        memcpy(pcr_cmd_buf + 14, sha1_alg, sizeof(sha1_alg));
                        printf("Read PCR %i (SHA-1):\n", pcr_index);
                }
                else if ('R' == option)
                {
                        memcpy(pcr_cmd_buf + 14, sha256_alg, sizeof(sha256_alg));
                        printf("Read PCR %i (SHA-256):\n", pcr_index);
                }
        } while (0);

        return ret_val;
}

int pcr_reset(char *pcr_index_str, uint8_t *pcr_cmd_buf)
{
        int ret_val = EXIT_SUCCESS; // Return value.
        uint8_t pcr_index = 0;      // PCR user input byte.

        do
        {
                NULL_POINTER_CHECK(pcr_index_str);
                NULL_POINTER_CHECK(pcr_cmd_buf);

                // Convert the command line input to bytes.
                if (1 != strlen(pcr_index_str) / 2 + strlen(pcr_index_str) % 2)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a single hex value (2 characters) without leading '0x'.\n");
                        break;
                }
                ret_val = hexstr_to_bytearray(pcr_index_str, &pcr_index, 1);
                RET_VAL_CHECK(ret_val);
                if (23 < pcr_index)
                {
                        ret_val = ERR_BAD_CMD;
                        fprintf(stderr, "Bad option. Enter a hex value between 0x00 and 0x17 in hex without leading '0x'.\n");
                        break;
                }

                // Copy command bytes.
                memcpy(pcr_cmd_buf, tpm2_pcr_reset, sizeof(tpm2_pcr_reset));

                // Store pcr_index at the correct byte index in the command byte stream.
                pcr_cmd_buf[13] = pcr_index;

                printf("Reset PCR %i:\n", pcr_index);
        } while (0);

        return ret_val;
}
