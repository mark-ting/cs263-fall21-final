#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sgx_utils.h>
#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>

#include "enclave.h"
#include "enclave_t.h"

int printf(const char *format, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list arg;
    va_start(arg, format);
    vsnprintf(buf, BUFSIZ, format, arg);
    va_end(arg);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

void hello_world() {
    printf("Hello World!\n"); 
}

sgx_status_t enclave_ra_init(sgx_ec256_public_t key, sgx_ra_context_t *ctx) {
    return sgx_ra_init(&key, 0, ctx);
}

sgx_status_t enclave_ra_close(sgx_ra_context_t ctx) {
    return sgx_ra_close(ctx);
}
