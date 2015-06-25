typedef struct
{
	uint8_t type;
	uint8_t indivAdr;
	uint8_t srcArea;
	uint8_t srcLine;
	uint8_t srcDev;
	uint8_t dst[2];
	uint8_t len;
	uint8_t payload[BUFSIZE];
}knxPacket;

int decodeFrame(uint8_t *, knxPacket *);
#define TRUE	1
#define FALSE	0
#define stdFrame	0
#define extFrame	1

// bit definitions
#define frameType	7
#define repeatedFrame	5

// field definitions
#define STDctrlField	0
#define STDsrcField1	1
#define STDsrcField2	2
#define STDdestField1	3
#define STDdestField2	4
#define STDlenField	5		// also contains AT + NCPI (=TTL)
#define STDapciField	6
#define STDpayloadField	7
//	MACROS
#define srcAreaAddress(frame)		(frame[STDsrcField1] >> 4)
#define srcLineAddress(frame)		((frame[STDsrcField1] & 0x0F))
#define srcDeviceAddress(frame)		(frame[STDsrcField2])

#define destAreaAddress(frame)		((frame[STDdestField1] >> 4 ))
#define destLineAddress(frame)		((frame[STDdestField1] & 0x0F))
#define destDeviceAddress(frame)	(frame[STDdestField2])

#define isRepeated(frame) ((frame[STDctrlField] >> repeatedFrame) ^ 0x01)

#define isStdFrame(frame) ((frame[STDctrlField] >> frameType) & 0x01)
#define isExtFrame(frame) ((frame[STDctrlField] >> frameType) ^ 0x01)

// Standard Frame specific macros
// addr type on different locations for std / ext frames
#define isGroupAddrStd(frame)		((frame[5] >> 7) & 0x01)
#define isIndivAddrStd(frame)		((frame[5] >> 7) ^ 0x01)
#define lengthStd(frame)		(frame[5] & 0x0F)
#define hopcountStd(frame)		((frame[5] >> 4)& 0x07)		// = NCPI
#define tpciStd(frame)			(frame[6] >> 2)			// see 03_03_4_ Transport Layer...pdf

// Extended Frame specific macros
// addr type on different locations for std / ext frames
#define isGroupAddrExt(frame)		((frame[1] >> 7) & 0x01)
#define isIndivAddrExt(frame)		((frame[1] >> 7) ^ 0x01)
