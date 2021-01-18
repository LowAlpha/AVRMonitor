/*____________________________________________________________________________*\
|
|  File:        main.c
|  Author:      M.J.Bauer
|  Originated:  Feb 2007   (target is ATmega128 using IAR tools)
|  Revised:     July 2011  (ported to ATmega16 using AVR-GCC tools)
|  Compiler:    GNU-AVR-GCC
|
|  Project:     Stand-alone 'AVR Operating System' (AVROS) with command line
|               user interface (CLI).
|
|  This module comprises the mainline and background tasks.
|  This version is customized for the Atmega328P MCU.
|  [Ref:  www.mjbauer.biz]
\*____________________________________________________________________________*/

// Port to Atmega328P for Arduino Jan 2021

#include  "system.h"
#include  "periph.h"
#include  "cmnd.h"


// Functions in main module...
void  doBackgroundTasks( void );
void  update_LED_chaser( void );


// Globals...
uint16  gwDebugFlags;
uint16  gwSystemError;

// Welcome message
const  char  psWelcome[]  PROGMEM = "\nAVROS : Arduino Debug Monitor : ";


int  main( void )
{
	initMCUports();             // do initialisation
	initMCUtimers();
	init_UART();
	hci_init();

	HEARTBEAT_LED_TOGL;         // light heartbeat LED

#if INTERACTIVE_ON_STARTUP     
	putstr_P( psWelcome );      // output msg to serial port
	version_cmd();
	hci_put_resp_term();        // prompt
#endif

	ENABLE_GLOBAL_IRQ;          // launch kernel loop

	
	while ( 1 )
	{
		hci_service();
		doBackgroundTasks();
	}
    return ( 1 );               // main() should not return!
}


void  doBackgroundTasks( void )
{
	if ( b5msecTaskReq )
	{
		// Place calls to 5mSec periodic tasks here
		//
//		wdt_reset();                // TODO: Watchdog handler
		b5msecTaskReq = 0;
	}
	if ( b50mSecTaskReq )
	{
		// Place calls to 50mSec periodic tasks here
		//
		update_LED_chaser();        // demo routine (optional)
		b50mSecTaskReq = 0;
	}
	if ( b500msecTaskReq )
	{
		// Place calls to 500mSec periodic tasks here
		//
		HEARTBEAT_LED_TOGL;
		b500msecTaskReq = 0;
	}
}


/*
|   Demo background task --
|   LED chaser routine for diagnostic 7-segment LED display.
|   The function is called every 50ms, but the actual task is executed
|   on every alternate call, to get the desired chaser speed.
*/

void  update_LED_chaser( void )
{
	static  uint8   bLedChaser = 0x01;  // Segment pattern (1 segs on)
	static  uint8   bAlt;

	if ( (bAlt++ & 1) == 0 ) return;    // True on alternate calls

	LED_7SEG_PORT = (LED_7SEG_PORT & 0xC0) | (bLedChaser & 0x3F);
	bLedChaser = (bLedChaser << 1);
	if ( bLedChaser == 0x40) bLedChaser = 0x1;
	
}
// end
