int initSec(void *);
int secRD(void *);
int secWR(void *);
void keyInit(void *);
void preparePacket(void *, uint8_t);
void time2Str(unsigned char *buf);

#define SYNCTIMEOUT_SEC 3
#define SYNC_RETRIES 3
#define JOINTIMEOUT_SEC 3
#define JOIN_RETRIES 3

#define DIGESTSIZE	256
#define BUFSIZE		256
#define PSKSIZE		32	// preshared key size
#define GKSIZE		32	// global key size [byte]
#define MACSIZE		4	// number of bytes actually used for appended MAC to save space

#define SECLINES	2

#define	STATE_INIT		0
#define	STATE_SYNC_REQ		1
#define STATE_SYNC_WAIT_RESP	2
#define STATE_RESET_CTR		3
#define STATE_READY		4

#define READEND		0
#define WRITEEND	1

/*
		ENCRYPTION / AUTHENTICATION MODES
*/
#define		NOSEC		(0<<4)
#define		AUTHONLY	(1<<4)
#define		AUTHENC		(2<<4)
#define		RES_SECMODE	(3<<4)

/*
		SECURITY HEADER
*/
#define		syncReq		0x02
#define		syncRes		0x03
#define		discReq		0x04
#define		discRes		0x05
#define		dataServ	0x08

/*
		KIND OF CIPHERS USED for ENCRYPTION
*/
#define		NONE		(0<<6)
#define		AES128		(1<<6)
#define		BLOWFISH	(2<<6)
#define		CIPH_RES	(3<<6)

#define         SETSEC_HEADER(enctype, pkgtype, sectype)(enctype | pkgtype | sectype)
