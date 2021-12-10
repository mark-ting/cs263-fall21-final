#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sgx_key_exchange.h>
#include <sgx_report.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <curl/curl.h>
#include <string>
#include <iostream>
#include "crypto.h"
#include "base64.h"
#include "rapidjson/document.h"

#define PORT 8080

using namespace rapidjson;
using namespace std;

// code adapted from https://github.com/intel/sgx-ra-sample/

static const sgx_ec256_public_t public_key = {
    {
        0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
    },
    {
        0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
    }
};

static const unsigned char private_key[32] = {
	0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
	0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
	0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
	0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
};

static const sgx_spid_t spid = {
    // A539C5BD990BF7EE6B1EA40AF0B246C6
    0xa5, 0x39, 0xc5, 0xbd, 0x99, 0x0b, 0xf7, 0xee,
    0x6b, 0x1e, 0xa4, 0x0a, 0xf0, 0xb2, 0x46, 0xc6
};

void reverse_bytes(void *dest, void *src, size_t len) {
	size_t i;
	char *sp= (char *)src;
	if ( len < 2 ) return;
	if ( src == dest ) {
		size_t j;

		for (i= 0, j= len-1; i<j; ++i, --j) {
			char t= sp[j];
			sp[j]= sp[i];
			sp[i]= t;
		}
	} else {
		char *dp= (char *) dest + len - 1;
		for (i= 0; i< len; ++i, ++sp, --dp) *dp= *sp;
	}
}

void print_bytes(unsigned char* obj, int sz) {
    for (int i = 0; i < sz; i++) {
        printf("%02x ", obj[i]);
    }    
    printf("\n");
}

size_t parse_json(char *ptr, size_t size, size_t nmemb, void *userdata) {
    Document *document = (Document *)userdata;
    document->Parse(ptr);
    return nmemb; 
}


int attest(int sockfd, unsigned char mrenclave[32], unsigned char sk[16], unsigned char mk[16]) {

    send(sockfd, &public_key, sizeof(public_key), 0);

    struct msg01_struct {
        uint32_t msg0_extended_epid_group_id;
		sgx_ra_msg1_t msg1;
    } msg01;

    read(sockfd, &msg01, sizeof(msg01)); 
    sgx_ra_msg1_t msg1 = msg01.msg1;

    if (msg01.msg0_extended_epid_group_id != 0) {
		printf("Invalid EPID Group ID\n");
		return -1;
	}

    sgx_ra_msg2_t msg2;

    // Generate session key Gb

    EVP_PKEY *Gb = key_generate();

    // Derive KDK from Ga in msg1 and Gb
    
    unsigned char *Gab_x;
	size_t slen;
	EVP_PKEY *Ga;
    unsigned char smk[16];
    unsigned char kdk[16];
	unsigned char cmackey[16];
    memset(kdk, 0, 16);
    memset(cmackey, 0, 16);

    Ga = key_from_sgx_ec256(&msg1.g_a); 
    Gab_x= key_shared_secret(Gb, Ga, &slen);
    reverse_bytes(Gab_x, Gab_x, slen);

    cmac128(cmackey, Gab_x, slen, kdk);

    cmac128(kdk, (unsigned char *)("\x01SMK\x00\x80\x00"), 7, smk);

    // Construct msg2

    key_to_sgx_ec256(&msg2.g_b, Gb);
    msg2.spid = spid;
    msg2.quote_type = 1;
    msg2.kdf_id = 1;
 
    // Normally we would check the revocation list here; however, we ignore this for simplicity in this proof-of-concept
    msg2.sig_rl_size = 0;

    unsigned char digest[32], r[32], s[32], gb_ga[128];
    memcpy(gb_ga, &msg2.g_b, 64);
    memcpy(&gb_ga[64], &msg1.g_a, 64);

    ecdsa_sign(gb_ga, 128, key_private_from_bytes(private_key), r, s, digest);
	reverse_bytes(&msg2.sign_gb_ga.x, r, 32);
	reverse_bytes(&msg2.sign_gb_ga.y, s, 32);

    cmac128(smk, (unsigned char *)&msg2, 148, (unsigned char *)&msg2.mac);

    send(sockfd, &msg2, sizeof(msg2), 0);
     
    // Process msg3
    
    sgx_ra_msg3_t *msg3;
	uint32_t msg3_sz;	
	
    read(sockfd, &msg3_sz, sizeof(uint32_t));
    msg3 = (sgx_ra_msg3_t *)malloc(msg3_sz);  
    read(sockfd, msg3, msg3_sz);

    // Verify msg1.g_a == msg3.g_a 
    if (CRYPTO_memcmp(&msg1.g_a, &msg3->g_a, sizeof(sgx_ec256_public_t))) {
        printf("ERROR: msg1.g_a and msg3.g_a keys don't match\n");
        free(msg3);
        return -1;
    }

    // Verify MAC
    sgx_mac_t vrfymac;
    cmac128(smk, (unsigned char *)&msg3->g_a,
		msg3_sz - sizeof(sgx_mac_t),
		(unsigned char *)vrfymac); 
    if (CRYPTO_memcmp(msg3->mac, vrfymac, sizeof(sgx_mac_t))) {
		printf("Failed to verify msg3 MAC\n");
		free(msg3);
		return -1;
	}

    // Verify quote->epid_group_id == msg1.gid
    sgx_quote_t *quote = (sgx_quote_t *)msg3->quote;
    sgx_report_body_t *report_body = (sgx_report_body_t *)&quote->report_body;
    if (memcmp(msg1.gid, &quote->epid_group_id, sizeof(sgx_epid_group_id_t))) {
		printf("Failed to verify EPID group id.\n");
		free(msg3);
		return -1;
	}

    // Verify that the first 64 bytes of the quote are SHA256(Ga||Gb||VK)
    unsigned char vk[16];
    unsigned char msg_rdata[144];
    unsigned char vfy_rdata[64];
    cmac128(kdk, (unsigned char *)("\x01VK\x00\x80\x00"), 6, vk);
    memcpy(msg_rdata, &msg1.g_a, 64);
	memcpy(&msg_rdata[64], &msg2.g_b, 64);
	memcpy(&msg_rdata[128], vk, 16);
    sha256_digest(msg_rdata, 144, vfy_rdata);
    if (CRYPTO_memcmp((void *)vfy_rdata, (void *)&report_body->report_data, 32)) {
        printf("Failed to verify report.\n");
        free(msg3);
        return -1;
	}

    // Verify report with IAS

    uint32_t quote_sz = msg3_sz - sizeof(sgx_ra_msg3_t);
    char *b64quote = base64_encode((char *)&msg3->quote, quote_sz);

    CURL *curl;
    curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.trustedservices.intel.com/sgx/dev/attestation/v4/report");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Ocp-Apim-Subscription-Key: af44c08bb2b94a3c868eb8eeb192b605");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    string body = "{ \"isvEnclaveQuote\" : \"";
    body += b64quote;
    body += "\" }";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());

    Document response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, parse_json);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        printf("Failed to reach IAS service.\n");
        free(msg3);
        return -1;
    }

    string quote_status = response["isvEnclaveQuoteStatus"].GetString();
    printf("Quote Status: %s\n", quote_status.c_str());
    if (!quote_status.compare("OK") 
            && !quote_status.compare("CONFIGURATION_NEEDED")
            && !quote_status.compare("SW_HARDENING_NEEDED")
            && !quote_status.compare("CONFIGURATION_AND_SW_HARDENING_NEEDED")) {
        printf("Enclave not trusted.\n");
        free(msg3);
        return -1;
    } 

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // Verify MRENCLAVE
    printf("Received MRENCLAVE: ");
    print_bytes((unsigned char *)&report_body->mr_enclave, 32);
    if (CRYPTO_memcmp((void *)&report_body->mr_enclave, (void *)mrenclave, 32)) {
        printf("MRENCLAVE did not match.\n");
        return -1;
    }

    printf("Attestation Success!\n");

    //cmac128(kdk, (unsigned char *)("\x01MK\x00\x80\x00"), 6, mk);
	//cmac128(kdk, (unsigned char *)("\x01SK\x00\x80\x00"), 6, sk);

    return 0;

}

// Arguments: server address, MRENCLAVE file
int main(int argc, char *argv[]) {

    curl_global_init(CURL_GLOBAL_ALL);

    if (argc < 3) {
        printf("Too few arguments\n");
        return -1;
    }

    unsigned char mrenclave[32];
    FILE *fp = fopen(argv[2], "rb");
    if (!fp) {
        printf("Failed to open MRENCLAVE file\n");
        return -1;
    }
    if (fread(mrenclave, 1, 32, fp) != 32) {
        printf("failed to read MRENCLAVE file\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Failed to create socket\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }

    string hello = "hello from client";
    send(sockfd, hello.c_str(), hello.size(), 0);
    char buffer[1024] = {0};
    read(sockfd, buffer, 1024);
    printf("%s\n", buffer);

    int attestation = attest(sockfd, mrenclave);
    send(sockfd, &attestation, sizeof(int), 0);
    
    close(sockfd);

    return 0;
}
