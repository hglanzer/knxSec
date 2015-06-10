struct threadEnvClr_t
{
	char *socketPath;
	char *addrStr;		// allowed device addesses: 1-15
	uint8_t addrInt;
};
struct threadEnvSec_t
{
	uint8_t localRDBuf[270];
	uint8_t id;
	uint8_t addrInt;
	char *addrStr;
	char *socketPath;
	EIBConnection *secFD;
	int Read2MasterPipe[2];
	uint8_t state;
	uint8_t retryCount;
	uint32_t globalCount;
	time_t now;
	// ssl stuff
	EVP_PKEY *hmacSignKey[SECLINES];
	EVP_PKEY *hmacVerifykey[SECLINES];
	// used keys
	uint8_t PSK[PSKSIZE];
	uint8_t globalKey[GKSIZE];
};
