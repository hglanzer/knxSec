typedef struct
{
	char *socketPath;
	char *addrStr;		// allowed device addesses: 1-15
	uint8_t addrInt;
	EIBConnection *clrFD;
}threadEnvClr_t;
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
