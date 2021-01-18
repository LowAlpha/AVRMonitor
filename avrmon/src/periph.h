/*
*   periph.h
*/
#ifndef  _PERIPH_H_
#define  _PERIPH_H_

#include "system.h"

#define  SERIAL_RX_BUF_SIZE        64     // Serial input FIFO buffer size
#define  MSEC_PER_TICK              1     // RTI Timer tick interval, msec
#define  TICKS_PER_200MSEC        200     // RTI Timer ticks in 200ms

#define  HALT(n)   { DISABLE_GLOBAL_IRQ; PORTC = n; while (1); }  // Debug aid

#define  ENABLE_TICK_TIMER   (TIMSK1 |= (1<<OCIE1A))
#define  DISABLE_TICK_TIMER  (TIMSK1 &= ~(1<<OCIE1A))
#define  HEARTBEAT_LED_TOGL  (PORTB ^= BIT_0)		// Arduino 
#define  LED_7SEG_PORT       (PORTC)                // 76 leds LED driven by PORTC
//#define  CLEAR_RESET_FLAGS   (MCUCSR &= ~0x1F)      // Clear the MCU hardware reset flags

#define  UART_RX_DATA_AVAIL      (UCSR0A & (1<<RXC0))
#define  UART_RX_READ_BYTE       (UDR0)
#define  UART_TX_READY           (UCSR0A & (1<<UDRE0))
#define  UART_TX_WRITE_BYTE(b)   (UDR0 = (b))


// Globals...
extern  bool    b5msecTaskReq;
extern  bool    b50mSecTaskReq;
extern  bool    b500msecTaskReq;


// Peripheral device driver functions

void    initMCUports( void );
void    initMCUtimers( void );
uint32  millisec_timer( void );

void    init_UART( void );
void    UART_RX_IRQctrl( bool );
void    serialRxBufferFlush( void );
bool    serialRxDataAvail( void );
uchar   getch( void );
uchar   putch( uchar b );

uint8   eeprom_read_byte( uint16 uwAddr );


#endif  /* _PERIPH_H_ */
