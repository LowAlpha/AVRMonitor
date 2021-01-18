/*
*   system.h  --  Platform and build dependencies are defined here
*/

#ifndef  _SYSTEM_H_
#define  _SYSTEM_H_

#define  BUILD_VER_MAJOR  1
#define  BUILD_VER_MINOR  2			// Port to Atmega328P for Arduino
#define  BUILD_VER_DEBUG  20

//--------------- Compiler-specific includes ----------------------------------
#include <avr/io.h>            // AVR-GCC auto-target I/O defs
#include <avr/interrupt.h>     // AVR-GCC interrupt handling defs
#include <avr/pgmspace.h>      // AVR-GCC program memory storage defs
#include <stdlib.h>
#include <ctype.h>
//-----------------------------------------------------------------------------
#include "gendef.h"            // Generic def's for embedded C

//--------------  B U I L D   O P T I O N S  ----------------------------------
#define  CLOCK_FREQ     (16000000UL)      // MCU XTAL clock frequency (Hz)
#define  UART_BAUDRATE  (19200)         // Set UART Baudrate
#define  DEBUG_BUILD    TRUE            // Maybe FALSE in final release
#define  INTERACTIVE_ON_STARTUP  TRUE   // Set mode for HCI comm's at startup
//-----------------------------------------------------------------------------

#define  LITTLE_ENDIAN  TRUE            // ATmega AVR is little-endian

#define  SET_DEBUG_FLAG(bm)   (gwDebugFlags |= bm)        // Debug aid

// TODO: Check ATmega16 bootloader block size and start address
//#define  PROGRAM_ENTRY_POINT     (0x0000)     // Application program start address
//#define  BOOTLDR_ENTRY_ADDRESS   (0x1E00)     // ATmega16 bootloader start address

// Generic aliases for AVR-GCC intrinsic functions
#define  ENABLE_GLOBAL_IRQ       sei()
#define  DISABLE_GLOBAL_IRQ      cli()


// -------------  Global variables  -------------------
extern  uint16  gwDebugFlags;
extern  uint16  gwSystemError;

// ------  Public functions in main module  -----------
void  doBackgroundTasks( void );


#endif  /* _SYSTEM_H_ */
