int initSec(void *);
int secReceive(void *);
int secSend(void *);
void keyInit(void *);

#define SYNCTIMEOUT_SEC 3
#define SYNC_RETRIES 3
#define JOINTIMEOUT_SEC 3
#define JOIN_RETRIES 3

#define BUFSIZE		256
#define PSKSIZE		256	// preshared key size
#define KSIZE		256	// global key size

#define SECLINES	2

#define	INIT		0
#define	SYNC		1
#define RESET		2
#define HELO		3
#define CHALLENGE	4
#define INSYNC		5

#define READEND		0
#define WRITEEND	1

/*
		TYPES OF PACKAGES
*/
#define		SYNC_REQ	0
#define		SYNC_RES	1

#define		JOIN_REQ	2
#define		JOIN_RES	3

#define		DISC_REQ	4
#define		DISC_RES	5

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
		ENCRYPTION / AUTHENTICATION MODES
*/
#define		NOSEC		(0<<4)
#define		AUTHONLY	(1<<4)
#define		AUTHENC		(2<<4)
#define		RES_SECMODE	(3<<4)

/*
		KIND OF CIPHERS USED for ENCRYPTION
*/
#define		NONE		(0<<6)
#define		AES128		(1<<6)
#define		BLOWFISH	(2<<6)
#define		CIPH_RES	(3<<6)

#define         SETSEC_HEADER(enctype, pkgtype, sectype)(enctype | pkgtype | sectype)
