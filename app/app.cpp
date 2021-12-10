#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sgx_urts.h>
#include <sgx_ukey_exchange.h>
#include <sgx_uae_epid.h>
#include <sgx_uae_quote_ex.h>

#include "app.h"
#include "enclave_u.h"


#define PORT 8080

sgx_enclave_id_t global_eid = 0;

void print_bytes(unsigned char* obj, int sz) {
    for (int i = 0; i < sz; i++) {
        printf("%02x ", obj[i]);
    }
    printf("\n");
}

int initialize_enclave(void) {  // TODO: save launch token to file

    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;

    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("Failed to create enclave (error %#x).\n", ret);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    printf("%s", str);
}

int setup_server(struct sockaddr_in addr) {

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        return -1;
    }

    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return -1;
    }

    listen(server_socket, 5);

    return server_socket;
    
}

// Code adapted from https://github.com/intel/sgx-ra-sample
int attest(sgx_enclave_id_t eid, int sockfd, sgx_ra_context_t* ra_ctx) {

    char buffer[BUFSIZ] = {0};

    sgx_ec256_public_t client_public_key;
    read(sockfd, &client_public_key, sizeof(client_public_key));

    sgx_status_t status;
    sgx_status_t sgxrv; // ECALL return value
    sgx_ra_msg1_t msg1;
	sgx_ra_msg2_t msg2; // usually msg2 should be variable length because of the revocation list; however, we ignore this for simplicity in this proof-of-concept
	sgx_ra_msg3_t *msg3 = NULL;	
	uint32_t msg0_extended_epid_group_id = 0;
	uint32_t msg3_sz;

    // msg0
    
    status = enclave_ra_init(eid, &sgxrv, client_public_key, ra_ctx);
    if (status != SGX_SUCCESS) {
        fprintf(stderr, "enclave_ra_init: %08x\n", status);
        return 0;
    }

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if ( status != SGX_SUCCESS) {
		enclave_ra_close(eid, &sgxrv, *ra_ctx);
		fprintf(stderr, "sgx_get_extended_epid_group_id: %08x\n", status);
		return 0;
	}

    // msg1

    status = sgx_ra_get_msg1(*ra_ctx, eid, sgx_ra_get_ga, &msg1);
	if (status != SGX_SUCCESS) {
		enclave_ra_close(eid, &sgxrv, *ra_ctx);
		fprintf(stderr, "sgx_ra_get_msg1: %08x\n", status);
		return 0;
	}

    send(sockfd, &msg0_extended_epid_group_id, sizeof(msg0_extended_epid_group_id), 0);
    send(sockfd, &msg1, sizeof(msg1), 0); 

    // msg3

    read(sockfd, &msg2, sizeof(sgx_ra_msg2_t));

    status = sgx_ra_proc_msg2(*ra_ctx, eid, 
            sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, &msg2, 
            sizeof(sgx_ra_msg2_t), &msg3, &msg3_sz);
    if (status != SGX_SUCCESS) {
		enclave_ra_close(eid, &sgxrv, *ra_ctx);
		fprintf(stderr, "sgx_ra_proc_msg2: %08x\n", status);
		return 0;
	}
 
    send(sockfd, &msg3_sz, sizeof(uint32_t), 0);
    send(sockfd, msg3, msg3_sz, 0);

    int success;
    read(sockfd, &success, sizeof(int));

    return success;

}

int SGX_CDECL main(int argc, char *argv[]) {

    (void)(argc);
    (void)(argv);
 
    if (initialize_enclave() < 0) {
        return -1;
    }

    hello_world(global_eid);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    socklen_t addrlen = sizeof(addr);

    int server_socket = setup_server(addr); 
    int client_socket = accept(server_socket, (struct sockaddr *)&addr, &addrlen);

    const char *hello = "hello from server";
    char buffer[1024] = {0};
    read(client_socket, buffer, 1024);
    printf("%s\n", buffer);
    send(client_socket, hello, strlen(hello), 0);

    sgx_ra_context_t ra_ctx = 0xdeadbeef;
    int attestation = attest(global_eid, client_socket, &ra_ctx);
    if (attestation) {
        printf("Attestation Success!\n");
        sgx_sha256_hash_t mkhash, skhash;
        enclave_ra_get_key_hash(global_eid, ra_ctx, &mkhash, &skhash);
        printf("MK hash: ");
        print_bytes((unsigned char*)mkhash, 32);
        printf("SK hash: ");
        print_bytes((unsigned char*)skhash, 32);
    } else {
        printf("Attestation Failed.\n"); 
    }

    close(client_socket);
    close(server_socket);

    sgx_destroy_enclave(global_eid);

    return 0;
}
