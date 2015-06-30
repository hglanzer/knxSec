typedef struct
{
	uint8_t indCount[INDCOUNTSIZE];
	eibaddr_t src;				// this is the SRC of the origin KNX cleartext package
	eibaddr_t dest;				// this is the DEST of the origin KNX cleartext package
	uint8_t *derivedKey;
	uint8_t active;
	uint8_t frame[BUFSIZE];
	uint8_t len;
	EC_KEY *pkey;				// this is MY key pair
	uint8_t myPubKey[DHPUBKSIZE];		// this is MY low-level public key
}activeDiscReq_t;


/*
	this struct is used to keep track of the individual counters for standard KNX devices with unique SRC
	to discard the duplicates produced by SEC0 + SEC1
*/
typedef struct
{
	uint32_t indCount;
	eibaddr_t srcEIB;				// this is the SRC of the origin KNX cleartext package
}indCtr_t;

/*
	CLR thread ENV
*/
typedef struct
{
	char *socketPath;
	char *addrStr;		// allowed device addesses: 1-15
	uint8_t addrInt;

	indCtr_t indCtr[10];

	EIBConnection *clrFD;
	//	FDs for pipe()
	int *CLR2Master1PipePtr[2];
	int *CLR2Master2PipePtr[2];
	int SECs2ClrPipe[2];
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
	activeDiscReq_t activeDiscReq[10];		// FIXME: this is static
	uint8_t secRDbuf[BUFSIZE];
	uint8_t secGlobalCount[GLOBALCOUNTSIZE];
	uint32_t secGlobalCountInt;
	uint8_t addrInt;
	fd_set set;			// used for pipe-timeout
	uint8_t state;
	uint8_t retryCount;
	//	FDs for pipe()
	int RD2MasterPipe[2];		// connection 
	int *SECs2ClrPipePtr[2];
}threadEnvSec_t;
