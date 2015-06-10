typedef unsigned char byte;

/* Returns 0 for success, non-0 otherwise */
int make_keys(EVP_PKEY** skey, EVP_PKEY** vkey);

/* Returns 0 for success, non-0 otherwise */
int generateHMAC(const byte* msg, size_t mlen, byte** sig, size_t* slen, EVP_PKEY* pkey);

/* Returns 0 for success, non-0 otherwise */
int verifyHMAC(const byte* msg, size_t mlen, const byte* sig, size_t slen, EVP_PKEY* pkey);

/* Prints a buffer to stdout. Label is optional */
void print_it(const char* label, const byte* buff, size_t len);

int hmacInit(EVP_PKEY**, EVP_PKEY**, size_t *, size_t *);
