#include "globals.h"
#include<openssl/err.h>

void handleErrors()
{
	int e;
	ERR_load_crypto_strings();

	while((e=ERR_get_error()) != 0)
	{
		printf("\n\t%s, lib=%s, func=%s, reason=%s\n\n", ERR_error_string(e, NULL), ERR_lib_error_string(e), ERR_func_error_string(e), ERR_reason_error_string(e));
	}
	
}

/*
	just prints buff in HEX
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
int hmacInit(EVP_PKEY** skey, EVP_PKEY** vkey, size_t* slen, size_t* vlen)
{
	int result = -1;
	OpenSSL_add_all_algorithms();

	*skey = NULL;
	*vkey = NULL;

	*slen = DIGESTSIZE;
	*vlen = DIGESTSIZE;

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
		strcpy((char *)hkey, "\x4B\x48\xAA\xBF\x4C\x84\x3E\xDC\x99\x01\x98\x16\x03\xE1\x32\xBE\x67\x06\x69\xD5\xAC\xA8\x27\x20\x2C\x26\x61\x5D\x17\xC8\x20\xE2");
	//	print_it("*** FIXME *** STATIC HMAC key\n", hkey, size);
	
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
	/*
		int i = 0;
		printf("\t\t this is generateHMAC, msg = ");
		for(i=0; i< mlen; i++)
			printf("%02x ", msg[i]);
	
		printf("\n");
	*/
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
            printf("EVP_DigestSignFinal failed, mismatched signature sizes %ld, %ld", (long int)req, (long int)*slen);
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
/*
		uint8_t i;
		printf("\n\tthis is verifyHMAC\n\tMSG = ");
		for(i=0; i< mlen; i++)
			printf("%02x ", msg[i]);
	
		printf("\n\tMAC = ");
		for(i=0; i<slen;i++)
			printf("%02x ", sig[i]);
		printf("\n");
*/
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

void genECpubKeyLow(EC_KEY *pkey, uint8_t *buf)
{
	EC_POINT *ecPoint = NULL;
	size_t ecPoint_size;
	uint16_t i=0;

	if(NULL == pkey)
	{
		printf("BAD: pkey == NULL\n\n");
	}

	/* Generate the private and public key */
	if(1 != EC_KEY_generate_key(pkey))
		handleErrors();

	// set to compressed form
	EC_KEY_set_conv_form(pkey, POINT_CONVERSION_COMPRESSED);

	ecPoint = (EC_POINT *) EC_KEY_get0_public_key(pkey);
	if(!ecPoint)
		handleErrors();

	ecPoint_size = EC_POINT_point2oct(EC_KEY_get0_group(pkey), ecPoint, EC_KEY_get_conv_form(pkey), buf, BUFSIZE, NULL);	
	if(!ecPoint_size)
		handleErrors();
	if(ecPoint_size != DHPUBKSIZE)
	{
		printf("\n\nDH PUBKEYSIZE CHANGED, = %d / FIXME\n", ecPoint_size);
			exit(-1);
	}
	for(i=0; i<ecPoint_size;i++)
		printf("%02X ", buf[i]);
	
	printf(" %dbyte, format %d\n", ecPoint_size, EC_KEY_get_conv_form(pkey));
	if(!EC_POINT_is_on_curve(EC_KEY_get0_group(pkey), ecPoint, NULL))
	{
		printf("ERROR: point not on curve\n");
		exit(-1);
	}
}

unsigned char *deriveSharedSecretLow(EC_KEY *pkey, uint8_t *peerPubKey)
{
	int field_size, i=0;
	size_t secret_len;
	unsigned char *secret;
	EC_POINT *peerEcPoint;
	EC_KEY *peerEcKey;
	
	for(i=0;i<33;i++)
		printf("%02X ", peerPubKey[i]);

	printf("\n");

	peerEcPoint = EC_POINT_new(EC_KEY_get0_group(pkey));
	if(peerEcPoint == NULL)
		handleErrors();
	sleep(1);
	peerEcKey = EC_KEY_new();
        if(!peerEcKey)
                handleErrors();

	printf("De-serializing, form = %d\n", EC_KEY_get_conv_form(pkey));
	/*	To deserialize the public key:
			Pass the octets to EC_POINT_oct2point() to get an EC_POINT.
			Pass the EC_POINT to EC_KEY_set_public_key() to get an EC_KEY.	*/
	if(!EC_POINT_oct2point(EC_KEY_get0_group(pkey), peerEcPoint, peerPubKey, (size_t)33, NULL))
	{
		handleErrors();
		return FALSE;
	}
	if(!EC_KEY_set_public_key(peerEcKey, peerEcPoint))
	{
		handleErrors();
		return FALSE;
	}

	/* Allocate the memory for the shared secret */
	if(NULL == (secret = OPENSSL_malloc(32))) handleErrors();
	//if(NULL == (secret = OPENSSL_malloc(&secret_len))) handleErrors();

	/* Derive the shared secret */
	secret_len = ECDH_compute_key(secret, 32, EC_KEY_get0_public_key(peerEcKey), pkey, NULL);

	if(&secret_len <= 0)
	{
		OPENSSL_free(secret);
		return NULL;
	}

	printf("derived: ");
	for(i=0; i<32;i++)
	{
		printf("%02X ", secret[i]);
	}
	printf("\n");

	return secret;

}
/*
	pkey is a pointer to this' side keypair
*/
void genECpubKey(EVP_PKEY *pkey, uint8_t *buf, EC_GROUP *group)
{
	EVP_PKEY_CTX *pctx;
	EVP_PKEY_CTX *kctx;
	int i =0;
	EVP_PKEY *params = NULL;
	EC_KEY * ecKey = NULL;
	EC_POINT *ecPoint = NULL;
	size_t ecPoint_size;

	//	Create the context for parameter generation 
	if(NULL == (pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL)))
		handleErrors();

	/*	Initialise the parameter generation	*/
	if(1 != EVP_PKEY_paramgen_init(pctx))
		handleErrors();

	/*	We're going to use the ANSI X9.62 Prime 256v1 curve	*/
	if(1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1))
		handleErrors();

	/*	Create the parameter object params */
	if (!EVP_PKEY_paramgen(pctx, &params))
		handleErrors();

	/*	Create the context for the key generation	*/
	if(NULL == (kctx = EVP_PKEY_CTX_new(params, NULL)))
		handleErrors();

	/*	Generate the key	*/
	if(1 != EVP_PKEY_keygen_init(kctx))
		handleErrors();

	if (1 != EVP_PKEY_keygen(kctx, &pkey))
		handleErrors();

/*	how to extract the public key...		needs group context...

		Pass the EVP_PKEY to EVP_PKEY_get1_EC_KEY() to get an EC_KEY.
		Pass the EC_KEY to EC_KEY_get0_public_key() to get an EC_POINT.
		Pass the EC_POINT to EC_POINT_point2oct() to get octets, which are just unsigned char *.

	use this to get point conversion form:
		point_conversion_form_t EC_GROUP_get_point_conversion_form(const EC_GROUP *);		*/

	// ecKey only used to obtain public key from EVP_PKEY
	ecKey = EC_KEY_new();
	if(!ecKey)
		handleErrors();
	// 	convert EVP key -> EC key
	ecKey = EVP_PKEY_get1_EC_KEY(pkey);
	if(!ecKey)
		handleErrors();

	// set to compressed form
	EC_KEY_set_conv_form(ecKey, POINT_CONVERSION_COMPRESSED);

	ecPoint = (EC_POINT *) EC_KEY_get0_public_key(ecKey);
	if(!ecPoint)
		handleErrors();

	ecPoint_size = EC_POINT_point2oct(EC_KEY_get0_group(ecKey), ecPoint, EC_KEY_get_conv_form(ecKey), buf, BUFSIZE, NULL);	
	if(!ecPoint_size)
		handleErrors();
	if(ecPoint_size != DHPUBKSIZE)
	{
		printf("\n\nDH PUBKEYSIZE CHANGED, = %d / FIXME\n", ecPoint_size);
			exit(-1);
	}
	for(i=0; i<ecPoint_size;i++)
		printf("%02X ", buf[i]);
	
	printf(" %dbyte, format %d\n", ecPoint_size, EC_KEY_get_conv_form(ecKey));
	if(!EC_POINT_is_on_curve(EC_KEY_get0_group(ecKey), ecPoint, NULL))
	{
		printf("ERROR: point not on curve\n");
		exit(-1);
	}

	BIGNUM *xCoord;
	BIGNUM *yCoord;
	xCoord = BN_new();
	yCoord = BN_new();

/*
	these functions are used to 'uncompress'	

	if(!EC_POINT_set_compressed_coordinates_GF2m(EC_KEY_get0_group(ecKey), ecPoint, xCoord, yBit, NULL))
	if(!EC_POINT_set_compressed_coordinates_GFp(EC_KEY_get0_group(ecKey), ecPoint, xCoord, yBit, NULL))
	{
		printf("compression failed\n");
	}
*/

	// these functions always return the 'fullsized' x / y coordinates, no matter of the conv_form

	//if (!EC_POINT_get_affine_coordinates_GF2m(EC_KEY_get0_group(ecKey), ecPoint, xCoord, yCoord, NULL))
	if (!EC_POINT_get_affine_coordinates_GFp(EC_KEY_get0_group(ecKey), ecPoint, xCoord, yCoord, NULL))
	{
		printf("get coord failed\n");
	}
	/*
	else
	{
		printf("     x = 0x");
		BN_print_fp(stdout, xCoord);
		printf("\n     y = 0x");
		BN_print_fp(stdout, yCoord);
		printf("\n");
	}
	*/
	
	BN_free(xCoord);
	BN_free(yCoord);

	EVP_PKEY_free(params);
	EVP_PKEY_CTX_free(kctx);
	EVP_PKEY_CTX_free(pctx);
	EC_KEY_free(ecKey);
}

unsigned char *deriveSharedSecret(EVP_PKEY *pkey, uint8_t *peerKey, size_t *secret_len, EC_GROUP *group)
{
	uint32_t i=0;
	EVP_PKEY_CTX *ctx;
	EC_KEY *peerEcKey;
	EC_KEY *myEcKey;
	EC_POINT *peerEcPoint;
	EVP_PKEY *peerEvpKey;
	unsigned char *secret;

	peerEcKey = EC_KEY_new();
	if(!peerEcKey)
		handleErrors();
	peerEvpKey = EVP_PKEY_new();
	if(!peerEvpKey)
		handleErrors();

	myEcKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if(!myEcKey)
		handleErrors();
	printf("Converting to EC\n");

	if(pkey == NULL)
		printf("pkey NULL SUCKZ\n");

	myEcKey = EVP_PKEY_get1_EC_KEY(pkey);
	if(!myEcKey)
		handleErrors();

	printf("get Point from Key...");
	peerEcPoint = EC_POINT_new(group);
	if(!peerEcPoint)
		handleErrors();

	printf("De-serializing...");

	/*	To deserialize the public key:

			Pass the octets to EC_POINT_oct2point() to get an EC_POINT.
			Pass the EC_POINT to EC_KEY_set_public_key() to get an EC_KEY.
			Pass the EC_KEY to EVP_PKEY_set1_EC_KEY to get an EVP_KEY.	*/
	if(!EC_POINT_oct2point(group, peerEcPoint, peerKey, 33, NULL))
	{
		handleErrors();
		return FALSE;
	}
	if(!EC_KEY_set_public_key(peerEcKey, peerEcPoint))
	{
		handleErrors();
		return FALSE;
	}
	if(!EVP_PKEY_set1_EC_KEY(peerEvpKey, peerEcKey))
	{
		handleErrors();
		return FALSE;
	}
	printf("Done\n");

	/* Create the context for the shared secret derivation */
	if(NULL == (ctx = EVP_PKEY_CTX_new(pkey, NULL)))
		handleErrors();
	/* Initialise */
	if(1 != EVP_PKEY_derive_init(ctx))
		handleErrors();
	/* Provide the peer public key */
	if(1 != EVP_PKEY_derive_set_peer(ctx, peerEvpKey))
		handleErrors();
	/* Determine buffer length for shared secret */
	if(1 != EVP_PKEY_derive(ctx, NULL, secret_len))
		handleErrors();
	/* Create the buffer */
	printf("OPENSSL_malloc()...");
	secret = OPENSSL_malloc(*secret_len);
	if(!secret)
	{
		printf("OPENSSL_malloc() failed\n");
		handleErrors();
	}
	printf("Done\n");
	/* Derive the shared secret */
	if((EVP_PKEY_derive(ctx, secret, secret_len)) <= 0)
		handleErrors();

	printf("len = %d, secret = ", *secret_len);
	for(i=0; i<*secret_len;i++)
		printf("%02X", secret[i]);
	printf("\n");

	/*	we got our secret - free unused memory	*/
	EVP_PKEY_CTX_free(ctx);
	EVP_PKEY_free(peerEvpKey);
	EVP_PKEY_free(pkey);

	/* Never use a derived secret directly. Typically it is passed
	 * through some hash function to produce a key */
	return secret;
}
