int decodeFrame(uint8_t *);
typdef struct
{
	uint8_t ctrl;
	uint8_t srcAreaLine;
	uint8_t srcDev;
	uint8_t dst[2];
	uint8_t atNcpiLength;
	uint8_t payload[25];
}knxStdPacket;
