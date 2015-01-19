#include <openssl/obj_mac.h>
#include<stdio.h>
#include<openssl/ec.h>
#include <openssl/evp.h>

void handleErrors()
{
	printf("something failed\n");
	exit(0);
}

int main()
{
	EVP_PKEY_CTX *pctx, *kctx;
	EVP_PKEY_CTX *ctx;
	unsigned char *secret;
	EVP_PKEY *pkey = NULL, *peerkey, *params = NULL;
	/* NB: assumes pkey, peerkey have been already set up */

	/* Create the context for parameter generation */
	if(NULL == (pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL))) handleErrors();

	/* Initialise the parameter generation */
	if(1 != EVP_PKEY_paramgen_init(pctx)) handleErrors();

	/* We're going to use the ANSI X9.62 Prime 256v1 curve */
	if(1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1)) handleErrors();

	/* Create the parameter object params */
	if (!EVP_PKEY_paramgen(pctx, &params)) handleErrors();

	/* Create the context for the key generation */
	if(NULL == (kctx = EVP_PKEY_CTX_new(params, NULL))) handleErrors();

	/* Generate the key */
	if(1 != EVP_PKEY_keygen_init(kctx)) handleErrors();
	if (1 != EVP_PKEY_keygen(kctx, &pkey)) handleErrors();

	/* Get the peer's public key, and provide the peer with our public key -
	 * how this is done will be specific to your circumstances */
	peerkey = get_peerkey(pkey);

	/* Create the context for the shared secret derivation */
	if(NULL == (ctx = EVP_PKEY_CTX_new(pkey, NULL))) handleErrors();

	/* Initialise */
	if(1 != EVP_PKEY_derive_init(ctx)) handleErrors();

	/* Provide the peer public key */
	if(1 != EVP_PKEY_derive_set_peer(ctx, peerkey)) handleErrors();

	/* Determine buffer length for shared secret */
	if(1 != EVP_PKEY_derive(ctx, NULL, secret_len)) handleErrors();

	/* Create the buffer */
	if(NULL == (secret = OPENSSL_malloc(*secret_len))) handleErrors();

	/* Derive the shared secret */
	if(1 != (EVP_PKEY_derive(ctx, secret, secret_len))) handleErrors();

	EVP_PKEY_CTX_free(ctx);
	EVP_PKEY_free(peerkey);
	EVP_PKEY_free(pkey);
	EVP_PKEY_CTX_free(kctx);
	EVP_PKEY_free(params);
	EVP_PKEY_CTX_free(pctx);

	/* Never use a derived secret directly. Typically it is passed
	 * through some hash function to produce a key */

	return 0;
}
