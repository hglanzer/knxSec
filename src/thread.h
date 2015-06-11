struct threadEnvClr_t
{
	char *socketPath;
	char *addrStr;		// allowed device addesses: 1-15
	uint8_t addrInt;
	EIBConnection *clrFD;
};
typedef struct
//struct threadEnvSec_t
{
	uint8_t id;
	char *socketPath;

	uint8_t addrInt;
	char *addrStr;
	int Read2MasterPipe[2];
	uint8_t state;
	uint8_t retryCount;
	uint8_t tmpsecWR;
	int MSGIDsecWR;
	int MSGIDsecMASTER;
}threadEnvSec_t;
