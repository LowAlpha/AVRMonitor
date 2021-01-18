/*____________________________________________________________________________*\
|
|  File:        periph.c
|  Author:      M.J.Bauer
|  Originated:  Feb 2007   (target ATmega128)
|  Revised:     July 2011  (ported to ATmega16)
|  Compiler:    GNU-AVR-GCC
|
|  Module comprises low-level hardware-dependent I/O interface functions.
|  This version is customized for the ATmega16 MCU.
\*____________________________________________________________________________*/

#include  "system.h"
#include  "periph.h"

/*____________________________________________________________________________*\
|
|   MCU device initialisation and on-chip peripheral driver functions
\*____________________________________________________________________________*/

bool    b5msecTaskReq;
bool    b50mSecTaskReq;
bool    b500msecTaskReq;

static  uint32  ulClockTicks;     // General-purpose "tick" counter


void  initMCUports( void )
{
//	DDRA  = 0xFF;             // Port A pins are outputs (LCD data port)
	DDRB  = 0x01;             // Port B pins PB0 is output  are outputs (keypad rows)
	DDRC  = 0xFF;             // Port C pins are outputs ( 6 leds display)
//	DDRD  = 0xBF;             // Port D pins are outputs, except PD6 (DEBUG button)
//	PORTD = BIT_6;            // Enable pullup on PD6 (DEBUG button input)
//	PORTB = 0x0F;             // Enable pullups on PB3:0 inputs (keypad cols)
}


/*
|   MCU timer/counter configuration --
|   Timer/counter #1 is set up to generate a 1ms periodic "tick" interrupt.
|   MCU clock frequency is defined by symbol CLOCK_FREQ (Hz) in system.h.
|   Acceptable values are 4000000 (4MHz), 8000000 (8MHz) or 16000000 (16MHz).
*/
void  initMCUtimers( void )
{
//	WDTCR = 0x0C;                       // TODO: Enable watchdog timer
	TCCR1B = 0x0A;                      // Mode = CTC; Prescale f/8 (Tc=1us @ 8MHz)

#if (CLOCK_FREQ == 4000000)
	OCR1AH = HI_BYTE( 500 );            // Load TOP register for 1ms Top count
	OCR1AL = LO_BYTE( 500 );
#elif  (CLOCK_FREQ == 8000000)
	OCR1AH = HI_BYTE( 1000 );           // Load TOP register for 1ms Top count
	OCR1AL = LO_BYTE( 1000 );
#else // Assume CLOCK_FREQ == 16000000
	OCR1AH = HI_BYTE( 2000 );           // Load TOP register for 1ms Top count
	OCR1AL = LO_BYTE( 2000 );
#endif

	ENABLE_TICK_TIMER;                  // Interrupt on Timer1 output compare
}


/*
|   INTERRUPT SERVICE ROUTINE ---  
|   Timer/Counter1 Compare channel-A.
|   RTI "Tick Handler" / task scheduler.
|   Short time-critical periodic tasks may be called within this ISR;
|   other periodic tasks are scheduled for execution in "background".
|   (See doBackgroundTasks() in main.c)
*/
ISR ( TIMER1_COMPA_vect )
{
	static  uint8   b500msecTimer = 0;
	static  uint8   b50mSecTimer = 0;
	static  uint8   b5mSecTimer = 0;

	ulClockTicks++;

	if ( ++b5mSecTimer >= 5 )
	{
		b5msecTaskReq = 1;
		b5mSecTimer = 0;
		b50mSecTimer++ ;
	}
	if ( b50mSecTimer >= 10 )
	{
		b50mSecTaskReq = 1;
		b50mSecTimer = 0;
		b500msecTimer++ ;
	}
	if ( b500msecTimer >= 10 )
	{
		b500msecTaskReq = 1;
		b500msecTimer = 0;
	}
}


/*
|   Return the value of ulClockTicks, which is incremented on every RTI tick.
|   For use as a general-purpose timer by functions outside of the Timer ISR.
*/
uint32  millisec_timer( void )
{
	uint32  ulTemp;

	DISABLE_TICK_TIMER;
	ulTemp = ulClockTicks;
	ENABLE_TICK_TIMER;

	return  ulTemp;
}


/*____________________________________________________________________________*\
|
|   UART support functions for serial port I/O.
|   See also UART I/O macros defined in periph.h
\*____________________________________________________________________________*/

static  uint8   acRx0buffer[SERIAL_RX_BUF_SIZE];
static  uint8  *pcRx0Head;          // Pointer to next available unread char
static  uint8  *pcRx0Tail;          // Pointer to next free place for writing
static  uint8   bRx0Count;          // Number of unread chars in RX buffer

/*
|   Initialise MCU UART and enable for polled I/O.
|   Called from main() before using serial port.
|   CLOCK_FREQ and UART_BAUDRATE are defined in system.h
|
|   Async data frame format: 8, N, 1 (data, parity, stop bits)
*/


void  init_UART( void )
{
	UBRR0H = (uchar) HI_BYTE( CLOCK_FREQ/(UART_BAUDRATE*16L) -1 );
	UBRR0L = (uchar) LO_BYTE( CLOCK_FREQ/(UART_BAUDRATE*16L) -1 );
	
	UCSR0A = ~(1<<U2X0);				   // x1 Mode
	
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);      // 8 bit no parity
	
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);        // Enable Receiver and Transmitter

	serialRxBufferFlush();                 // Flush the serial RX FIFO buffer
}


/*
|	UART receiver interrupt control.
|	Masks or unmasks RXC IRQ without affecting global interrupt status.
|
|	Args:	bool yIRQmask = {0 | 1};  0 => RX INT is masked (disabled)
*/
void  UART_RX_IRQctrl( bool yIRQenab )
{
	if ( yIRQenab ) UCSR0B |= (1<<RXCIE0);    // RX INT enabled
	else  UCSR0B &= ~(1<<RXCIE0);             // RX INT disabled
}


/*
|   INTERRUPT SERVICE ROUTINE --- UART Receiver RX Complete ---
|	The IRQ signals that one or more bytes have been received by the UART;
|	the byte(s) are read out of the UART RX data register(s) and stored
|	in the serial input RX buffer in SRAM (circular FIFO).
*/
ISR ( USART0_RX_vect ) 
{
    while ( UART_RX_DATA_AVAIL )
    {
		if ( bRx0Count < SERIAL_RX_BUF_SIZE )
		{
			*pcRx0Tail++ = UART_RX_READ_BYTE;
			if ( pcRx0Tail >= (acRx0buffer + SERIAL_RX_BUF_SIZE) )   // Wrap
				pcRx0Tail = acRx0buffer;
			bRx0Count++;
		}
    }
}

void  serialRxBufferFlush( void )
{
	UART_RX_IRQctrl( DISABLE );
	pcRx0Tail = acRx0buffer;
	pcRx0Head = acRx0buffer;
	bRx0Count = 0;
	UART_RX_IRQctrl( ENABLE );
}


/*
|   Function checks for data available in the serial input RX FIFO buffer.
|   If so, it returns TRUE.
*/
bool  serialRxDataAvail( void )
{
	return  ( bRx0Count != 0 );
}

/*
|   getch() - Fetch next unread char from serial RX FIFO buffer.
|
|   The function does not wait for data available in the RX buffer;
|   the caller should first check using serialRxDataAvail().
|   If there is no data available, the function returns NUL (0).
|   The fetched char is not echoed back to the serial output stream.
|
|   Returns:  Byte from serial RX buffer (or 0, if buffer is empty).
*/
uchar  getch( void )
{
	char  b = 0;

	if ( bRx0Count )
	{
		UART_RX_IRQctrl( DISABLE );
		b = *pcRx0Head++;               // Fetch char from buffer
		--bRx0Count;
		if ( pcRx0Head >= (acRx0buffer + SERIAL_RX_BUF_SIZE) )
			pcRx0Head = acRx0buffer;    // Wrap pointer
		UART_RX_IRQctrl( ENABLE );
	}
	return  b;
}


/*
|   putch(c) - Output single char to serial port.
|
|   Waits for UART transmitter ready, then writes the char to the TX data register.
|   While waiting, any pending background tasks are delayed;
|   any currently executing task is suspended.
|
|   Entry args: (uint8) b = TX byte
|   Returns:    (uint8) b = TX byte
*/
uchar  putch( uchar b )
{
	do { /* wait */ } until ( UART_TX_READY );

	UART_TX_WRITE_BYTE( b );

	return  b;
}


/*____________________________________________________________________________*\
|
|   EEPROM SUPPORT FUNCTIONS
|   See also EEPROM handling macros defined in periph.h
\*____________________________________________________________________________*/
/*
|   eeprom_read_byte() - Get byte from EEPROM at offset uwAddr.
|
|   Entry args: (uint16) uwAddr = EEPROM address (offset)
|   Returns:    (uint8) bDat = value of byte read
*/
uint8  eeprom_read_byte( uint16 uwAddr )
{
	return  0xFF;   // TODO: return byte from EEPROM at uwAddr
}

// end
