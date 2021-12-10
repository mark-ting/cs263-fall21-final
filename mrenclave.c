/*

Copyright 2019 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they
were provided to you (License). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit
this software or the related documents without Intel's prior written
permission.

This software and the related documents are provided as is, with no
express or implied warranties, other than those that are expressly stated
in the License.

*/

/*
 * Read in a SIGSTRUCT dump file (from: sgx_sign dump -cssfile ...) and
 * produce the MRENCLAVE hash.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <getopt.h>
#include <unistd.h>

/*
 * From the "Intel(r) 64 and IA-32 Architectures Software Developer 
 * Manual, Volume 3: System Programming Guide", Chapter 38, Section 13,
 * Table 38-19 "Layout of Enclave Signature Structure (SIGSTRUCT)"
 */

#define MODULUS_OFFSET	128
#define MODULUS_SIZE	384
#define ENCLAVEHASH_OFFSET	960
#define ENCLAVEHASH_SIZE	32

int main(int argc, char **argv) {

	char *cssfile= NULL;
	char *sigstruct_raw= NULL;
	unsigned char modulus[MODULUS_SIZE];
	unsigned char mrsigner[32]; /* Size of SHA-256 hash */
	unsigned char mrenclave[ENCLAVEHASH_SIZE]; /* Size of SHA-256 hash */
	FILE *fp;
	size_t bread;	

    if (argc != 2) {
        exit(1);
    }
    cssfile = argv[1];

	fp= fopen(cssfile, "r");
	if ( fp == NULL ) {
		fprintf(stderr, "%s: ", cssfile);
		perror("fopen");
		exit(1);
	}

    /* Seek to the location of the enclave hash (mrenclave) */

	if (fseek(fp, ENCLAVEHASH_OFFSET, SEEK_SET) == -1) {
	    fprintf(stderr, "%s: ", cssfile);
	    perror("fseek");
	    exit(1);
	}

	/* Read the enclave hash (mrenclave) */

	bread = fread(mrenclave, 1, (size_t) ENCLAVEHASH_SIZE, fp);
	if ( bread != ENCLAVEHASH_SIZE ) {
	    fprintf(stderr, "%s: not a valid sigstruct (file too small)\n", cssfile);
	    exit(1);
	}

	fclose(fp);

    fp = fopen("./mrenclave.bin", "wb");
    if ( fp == NULL ) {
		fprintf(stderr, "mrenclave.bin: ");
		perror("fopen");
		exit(1);
	}

    fwrite(mrenclave, sizeof(unsigned char), ENCLAVEHASH_SIZE, fp);

    fclose(fp);

    printf("mrenclave: ");
    for (int i = 0; i < 32; i++) {
        printf("%02x ", mrenclave[i]);
    }
    printf("\n");
    
	exit(0);

}

