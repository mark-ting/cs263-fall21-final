#include <stdio.h>
#include <stdlib.h>

#include "sgx_error.h"      /* sgx_status_t */
#include "sgx_eid.h"        /* sgx_enclave_id_t */

#define TOKEN_FILENAME      "enclave.token"
#define ENCLAVE_FILENAME    "enclave.signed.so"

extern sgx_enclave_id_t global_eid;     /* global enclave id */
