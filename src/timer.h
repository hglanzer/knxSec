
#define TIMER0		0
#define TIMER1		1
#define TIMER2		2
#define TIMER3		3

#define BASE		0x20000000
#define TIMER_BASE	0x00003000
#define TIMER_IRQ	0x0000B210
#define TIMER_CTRL_OFFS	0x00
#define TIMER_LOW_OFFS	0x04
#define TIMER_HIGH_OFFS	0x08

#define BLOCK_SIZE	4*1024

//	----------------- MAKROS

#define TIMER_MATCH(adr, timer) (*adr & (1 << timer))
#define SET_BIT(adr, bit) (*adr = *adr | (1 << bit))
#define CLEAR_BIT(adr, bit) (*adr = *adr & (1 << bit))
