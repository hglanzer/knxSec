typedef struct
{
	uint32_t indCount;
	eibaddr_t src;				// this is the SRC of the origin KNX cleartext package
	eibaddr_t dest;				// this is the DEST of the origin KNX cleartext package
	uint8_t *derivedKey;
	uint8_t active;
	uint8_t frame[BUFSIZE];
	uint8_t len;
	EC_KEY *pkey;				// this is MY key pair
	uint8_t myPubKey[DHPUBKSIZE];		// this is MY low-level public key
	//uint8_t peerPubKey[DHPUBKSIZE];		// this is the PEER low-level public key
}indCounters_t;

/*
	CLR thread ENV
*/
typedef struct
{
	char *socketPath;
	char *addrStr;		// allowed device addesses: 1-15
	uint8_t addrInt;
	EIBConnection *clrFD;
	int *CLR2Master1PipePtr[2];
	int *CLR2Master2PipePtr[2];
}threadEnvClr_t;

/*
	SEC threads ENV
*/
typedef struct
{
	uint8_t id;
	char *socketPath;
	EIBConnection *secFDWR;
	EIBConnection *secFDRD;
	EVP_PKEY *skey;
	EVP_PKEY *vkey;
	size_t slen;
	size_t vlen;
	time_t now;
	indCounters_t indCounters[10];		// FIXME: this is static
	uint8_t secRDbuf[BUFSIZE];
	uint8_t secGlobalCount[GLOBALCOUNTSIZE];
	uint32_t secGlobalCountInt;
	uint8_t addrInt;
	fd_set set;			// used for pipe-timeout
	int RD2MasterPipe[2];
	uint8_t state;
	uint8_t retryCount;
	uint8_t tmpsecWR;
	int MSGIDsecWR;
	int MSGIDsecMASTER;
}threadEnvSec_t;
