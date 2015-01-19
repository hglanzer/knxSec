#include<stdio.h>

/*
                TYPES OF PACKAGES
*/
#define         SYNC_REQ        0
#define         SYNC_RES        1

#define         JOIN_REQ        2
#define         JOIN_RES        3

#define         DISC_REQ        4
#define         DISC_RES        5

#define         DATA_SRV        6
#define         RES1            7
#define         RES2            8
#define         RES3            9
#define         RES4            0xA
#define         RES5            0xB
#define         RES6            0xC
#define         RES7            0xD
#define         RES8            0xE
#define         RES9            0xF

/*
                ENCRYPTION / AUTHENTICATION MODES
*/
#define         NOSEC           (0<<4)
#define         AUTHONLY        (1<<4)
#define         AUTHENC         (2<<4)
#define         RES_SECMODE     (3<<4)

/*
                KIND OF CIPHERS USED for ENCRYPTION
*/
#define         NONE            (0<<6)
#define         AES128          (1<<6)
#define         BLOWFISH        (2<<6)

#define         SEC(enctype, pkgtype, sectype)(enctype | pkgtype | sectype)


int main(void)
{
	printf("SYNC REQ:\n");
	printf("%2X\n", SEC(NONE, SYNC_REQ, NOSEC));
	printf("SYNC RESPONSE:\n");
	printf("%2X\n", SEC(NONE, SYNC_RES, AUTHONLY));
	
	printf("JOIN REQ:\n");
	printf("%2X\n", SEC(NONE, JOIN_REQ, AUTHONLY));
	printf("JOIN RESPONSE:\n");
	printf("%2X\n", SEC(AES128, JOIN_RES, AUTHENC));
	
	printf("DISC REQ:\n");
	printf("%2X\n", SEC(BLOWFISH, DISC_REQ, AUTHENC));
	printf("DISC RESPONSE:\n");
	printf("%2X\n", SEC(BLOWFISH, DISC_RES, AUTHENC));

	printf("DATA AES:\n");
	printf("%2X\n", SEC(AES128, DATA_SRV, AUTHENC));
	printf("DATA BLOWFISH:\n");
	printf("%2X\n", SEC(BLOWFISH, DATA_SRV, AUTHENC));
	return 0;
}
