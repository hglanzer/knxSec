void updateGlobalCount(void *);
int checkFreshness(void *, uint8_t *);
void time2Str(void *, unsigned char *);
int initSec(void *);
int secWRnew(char *, uint8_t , uint8_t , void *);
int secWR(void *);
void secRD(void *);
void keyInit(void *);

#define SYNCTIMEOUT_SEC 3
#define SYNC_RETRIES 3
#define JOINTIMEOUT_SEC 3
#define JOIN_RETRIES 3

#define BUFSIZE		256
#define PSKSIZE		32	// preshared key size
#define GKSIZE		32	// global key size [byte]
#define MACSIZE		4
#define DIGESTSIZE	256
#define GLOBALCOUNTSIZE	4

#define SECLINES	2

#define	STATE_INIT		0
#define	STATE_SYNC_REQ		1
#define STATE_SYNC_WAIT_RESP	2
#define STATE_RESET_CTR	3
#define STATE_READY		4

#define READEND		0
#define WRITEEND	1

/*
		MAKROS for EIB Addr
*/
#define AreaAddress(adr) ((uint8_t)(adr >> 4 ))
#define LineAddress(adr) ((uint8_t)(adr & 0x0F))
#define EIBADDR(area,line,device) (((area << 4 | line) << 8) | device)

/*
		TYPES OF PACKAGES
*/
#define		syncReq		0x02
#define		syncRes		0x03
#define		discReq		0x04
#define		discRes		0x05

#define		DATA_SRV	6
#define		RES1		7
#define		RES2		8
#define		RES3		9
#define		RES4		0xA
#define		RES5		0xB
#define		RES6		0xC
#define		RES7		0xD
#define		RES8		0xE
#define		INVALID		0xF

/*
		KIND OF CIPHERS USED for ENCRYPTION
*/
#define		NONE		(0<<6)
#define		AES128		(1<<6)
#define		BLOWFISH	(2<<6)
#define		CIPH_RES	(3<<6)

#define         SETSEC_HEADER(enctype, pkgtype, sectype)(enctype | pkgtype | sectype)
