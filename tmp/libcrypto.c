#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <gmp.h>

BN_GENCB * callback(BN_GENCB *a, int b, int c)
{
	printf("this is callback\n");
	return a;
}

int main(int arc, char *argv[])
{ 
	DH *dh = DH_new();

	BN_GENCB *(*cb)(BN_GENCB *, int, int);	
	cb = &callback;

	char buf[400];
	char *bufPtr = &buf[0];

	/* Load the human readable error strings for libcrypto */
	ERR_load_crypto_strings();
	/* Load all digest and cipher algorithms */
	OpenSSL_add_all_algorithms();
	/* Load config file, and other important initialisation */
	OPENSSL_config(NULL);

	/*
		use special PRNG if possible:
			* /dev/random
			* /dev/hwrng
			* ...
	*/

	/*
 		----------------------------	PARAMATER GEN	------------------------
	*/

	printf("start generation\n");
	DH_generate_parameters_ex(dh, 256, 2, NULL);
	printf("paramters DONE\n");

	if(dh->pub_key == NULL)
		printf("pubkey init to NULL\n");

	DH_generate_key(dh);
	printf("key DONE\n");
	bufPtr = BN_bn2hex(dh->p);
	printf ("prime    : %s\n", bufPtr);
	bufPtr = BN_bn2hex(dh->g);
	printf ("generator: %s\n", bufPtr);
	bufPtr = BN_bn2hex(dh->pub_key);
	printf ("pubkey   : %s\n", bufPtr);
	bufPtr = BN_bn2hex(dh->priv_key);
	printf ("privkey  : %s\n", bufPtr);

  /* Clean up */

  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();

  return 0;
}
