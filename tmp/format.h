#define		SYNC_REQ	0
#define		SYNC_RES	1

#define		JOIN_REQ	2
#define		JOIN_RES	3

#define		DISC_REQ	4
#define		DISC_RES	5

#define		NOSEC		(0<<4)
#define		AUTHONLY	(1<<4)
#define		AUTHENC		(2<<4)

#define         SETSEC_HEADER(pkgtype, sectype)(pkgtype | sectype)
