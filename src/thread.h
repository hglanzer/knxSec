struct threadEnvClr_t
{
	char *socket;
};
struct threadEnvSec_t
{
	uint8_t localRDBuf[270];
	uint8_t id;
	char *socket;
	int Read2MasterPipe[2];
	uint8_t state;
	uint8_t retryCount;
	uint32_t globalCount;
	// used keys
	uint8_t PSK[PSKSIZE];
	uint8_t globalKey[GKSIZE];
};
