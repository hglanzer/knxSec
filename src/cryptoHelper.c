#include "globals.h"

/*
*/
void print_it(const char* label, const byte* buff, size_t len)
{
	size_t i = 0;
	if(!buff || !len)
		return;

	if(label)
		printf("%s: ", label);

	for(i=0; i < len; ++i)
		printf("%02X", buff[i]);

	printf("\n");
}

/*
	generates two keys: signkey, verificationkey
	returns	0 if OK
		-1 if failure occured
*/
int hmacInit(EVP_PKEY** skey, EVP_PKEY** vkey)
{
	int result = -1;
	OpenSSL_add_all_algorithms();

	*skey = NULL;
	*vkey = NULL;

/// START OF MAKE_KEYS	
	/*
		HMAC key - using static key
	*/
	byte hkey[EVP_MAX_MD_SIZE];

	if(!skey || !vkey)
		return -1;

	// free skey/vkey if not fresh
	if(*skey != NULL)
	{
		EVP_PKEY_free(*skey);
		*skey = NULL;
	}

	if(*vkey != NULL)
	{
		EVP_PKEY_free(*vkey);
		*vkey = NULL;
	}

	do
	{
		const EVP_MD* md = EVP_get_digestbyname("SHA256");
		//const EVP_MD* md = EVP_get_digestbyname(hn);
		assert(md != NULL);
		if(md == NULL)
		{
			printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
			break; /* failed */
		}

		int size = EVP_MD_size(md);
		assert(size >= 16);
		if(!(size >= 16))
		{
			printf("EVP_MD_size failed, error 0x%lx\n", ERR_get_error());
			break; /* failed */
		}

		assert(size <= sizeof(hkey));
		if(!(size <= sizeof(hkey)))
		{
			printf("EVP_MD_size is too large\n");
			break; /* failed */
		}

		/*	Generate random bytes for hkey or
			use static hkey as base for signing/verification key for HMAC
		*/
		strcpy(hkey, "\x4B\x48\xAA\xBF\x4C\x84\x3E\xDC\x99\x01\x98\x16\x03\xE1\x32\xBE\x67\x06\x69\xD5\xAC\xA8\x27\x20\x2C\x26\x61\x5D\x17\xC8\x20\xE2");
		print_it("*** FIXME *** STATIC HMAC key", hkey, size);
	
		*skey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
		assert(*skey != NULL);
		if(*skey == NULL)
		{
			printf("EVP_PKEY_new_mac_key failed, error 0x%lx\n", ERR_get_error());
			break;
		}

		*vkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
		assert(*vkey != NULL);
		if(*vkey == NULL)
		{
			printf("EVP_PKEY_new_mac_key failed, error 0x%lx\n", ERR_get_error());
			break;
		}

		result = 0;
	} while(0);

	// hkey not needed anymore, we have what we need(skey, vkey)
	OPENSSL_cleanse(hkey, sizeof(hkey));
	
	assert(skey != NULL);
	if(skey == NULL)
		exit(1);

	assert(vkey != NULL);
	if(vkey == NULL)
		exit(1);

	// flip result	
	return !!result;
}

/*
*/
int generateHMAC(const byte* msg, size_t mlen, byte** sig, size_t* slen, EVP_PKEY* pkey)
{
	#ifdef DEBUG
	int i = 0;
	printf("\t\t this is generateHMAC, msg = ");
	for(i=0; i< mlen; i++)
		printf("%x ", msg[i]);

	printf("\n");
	#endif

    int result = -1;
    if(!msg || !mlen || !sig || !pkey) {
        assert(0);
        return -1;
    }
    if(*sig)
        OPENSSL_free(*sig);
    
    *sig = NULL;
    *slen = 0;
    
    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        assert(ctx != NULL);
        if(ctx == NULL) {
            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        const EVP_MD* md = EVP_get_digestbyname("SHA256");
        assert(md != NULL);
        if(md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        size_t req = 0;
        rc = EVP_DigestSignFinal(ctx, NULL, &req);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed (1), error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        assert(req > 0);
        if(!(req > 0)) {
            printf("EVP_DigestSignFinal failed (2), error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        *sig = OPENSSL_malloc(req);
        assert(*sig != NULL);
        if(*sig == NULL) {
            printf("OPENSSL_malloc failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        *slen = req;
        rc = EVP_DigestSignFinal(ctx, *sig, slen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed (3), return code %d, error 0x%lx\n", rc, ERR_get_error());
            break; /* failed */
        }
        
        assert(req == *slen);
        if(rc != 1) {
            printf("EVP_DigestSignFinal failed, mismatched signature sizes %ld, %ld", req, *slen);
            break; /* failed */
        }
        
        result = 0;
        
    } while(0);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }
    
    /* Convert to 0/1 result */
    return !!result;
}

/*

*/
int verifyHMAC(const byte* msg, size_t mlen, const byte* sig, size_t slen, EVP_PKEY* pkey)
{
    /* Returned to caller */
    int result = -1;
    
    if(!msg || !mlen || !sig || !slen || !pkey) {
        assert(0);
        return -1;
    }

    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        assert(ctx != NULL);
        if(ctx == NULL) {
            printf("EVP_MD_CTX_create failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        const EVP_MD* md = EVP_get_digestbyname("SHA256");
        assert(md != NULL);
        if(md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestInit_ex failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignInit failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestSignUpdate failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
       	
        byte buff[EVP_MAX_MD_SIZE];
        size_t size = sizeof(buff);
        
        rc = EVP_DigestSignFinal(ctx, buff, &size);
        assert(rc == 1);
        if(rc != 1) {
            printf("EVP_DigestVerifyFinal failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }
        
        assert(size > 0);
        if(!(size > 0)) {
            printf("EVP_DigestSignFinal failed (2)\n");
            break; /* failed */
        }
        
        const size_t m = (slen < size ? slen : size);
        result = !!CRYPTO_memcmp(sig, buff, m);
        
        OPENSSL_cleanse(buff, sizeof(buff));
        
    } while(0);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }
    
    /* Convert to 0/1 result */
    return !!result;
}
