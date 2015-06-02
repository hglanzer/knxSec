int decodeFrame(unsigned char *);
struct packetStruct
{
	uint8_t ctrl;
	uint8_t srcAreaLine;
	uint8_t srcDev;
	uint8_t dst[2];
	uint8_t atNcpiLength;
	uint8_t payload[25];
};
