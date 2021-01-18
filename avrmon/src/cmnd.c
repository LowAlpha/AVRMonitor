/*____________________________________________________________________________*\
|
|  File:        cmnd.c
|  Author:      M.J.Bauer
|  Originated:  Feb 2007   (target ATmega128)
|  Revised:     July 2011  (ported to ATmega16)
|  Compiler:    GNU-AVR-GCC
|
|  Project:     Stand-alone 'AVR Operating System' (debug monitor).
|               This version is customized for the ATmega16 MCU.
|
|  This source module implements the "host command interface" (HCI) and a
|  set of generic command functions, including resident "debug" commands.
|
|  A command string is composed of a 2-letter command "name" and a number of
|  user-supplied "arguments" (parameters). Some commands have no arguments.
|  A single space must be inserted between command buffer arguments, where
|  there is more than one (including the 2-letter command name).
|
|  The HCI is intended primarily for automatic machine-machine communication.
|  When using the HCI interactively, i.e. manually, with a terminal emulator
|  in full-duplex mode, character echo-back can be enabled in one of two ways:
|  (1) by sending the command "IU 1" (after which chars will be echoed), or
|  (2) by setting up your PC terminal emulator for "local echo".
|  While interactive mode is enabled, the "response char" (prompt) is '=';
|  otherwise it is '-' (hyphen).
|
|  Refer to "list" command strings for command syntax details.
|
|  [Ref:  www.mjbauer.biz]
\*____________________________________________________________________________*/

#include  "system.h"
#include  "periph.h"
#include  "cmnd.h"


// Command table entry looks like this
struct  CmndTableEntry_t
{
	char     cName1;            // command name, 1st char
	char     cName2;            // command name, 2nd char
	pfnvoid  Function;          // pointer to HCI function
};


static  char    gacCmdMsg[CMD_MSG_SIZE+1];      // Command message buffer
static  char  * pcCmdPtr;               // Pointer into gacCmdMsg[]
static  char    cRespCode;              // Response termination code
static  bool    yInteractive;


/*****
|   Command table -- maximum number of commands is 250.
|   (Application-specific command functions should go at the top)
*/
const  struct  CmndTableEntry_t  asCommand[] =
{
	{ 'D','P',    default_params_cmd },
	{ 'L','S',    list_cmd           },
	{ 'I','M',    interactive_cmd    },
	{ 'V','N',    version_cmd        },
	{ 'W','D',    watch_data_cmd     },
	{ 'S','E',    show_errors_cmd    },
	{ 'S','F',    show_flags_cmd     },
	{ 'R','S',    reset_MCU_cmd      },
	{ 'D','C',    dump_memory_cmd    },
	{ 'D','D',    dump_memory_cmd    },
	{ 'D','E',    dump_memory_cmd    },
	{ 'R','M',    read_data_mem_cmd  },
	{ 'W','M',    write_data_mem_cmd },
	{ 'I','P',    input_IOreg_cmd    },
	{ 'O','P',    output_IOreg_cmd   },
	{ 'E','E',    erase_eeprom_cmd   },
	{ '$','$',    null_cmd           }      // Last entry in cmd table
} ;


/*
|   Initialise the Host Command Interface
|   Called from main before HCI is used.
*/
void  hci_init(void)
{
#if INTERACTIVE_ON_STARTUP      // Set default HCI comm's mode
	yInteractive = TRUE;           
#else
	yInteractive = FALSE;
#endif
	hci_clear_command();
}


/*
|   Function:   hci_service
|   Purpose:    Host command interface (HCI) service routine.
|
|   This function is called *frequently* from the main "background" loop.
|   The function checks for RX data from the HCI input port; it returns
|   immediately if there's no new input data available from the input stream.
|   If there is data available, it is processed.
*/
void  hci_service( void )
{
	char   c;

	if ( serialRxDataAvail() )
	{
		c = getch();                // Fetch char; no echo yet
		hci_process_input( c );
	}
}


/*
|   Function examines a character received via the HCI input stream and decides what
|   to do with it. If printable, it goes into the command buffer, gacCmdMsg[].
|   When a command terminator (CR) is received, the command message is interpreted;
|   if the command name is identified, the respective command function is executed.
|   Command functions may generate response data to be transmitted back to the host PC.
*/
void  hci_process_input( char c )
{
	if ( c == '\r' )      // CR is the command message terminator
	{
		if ( pcCmdPtr != gacCmdMsg )    // if the command msg is not empty...
		{
			hci_exec_command();         // ... interpret the command.
		}
		else  hci_put_resp_term();
	}
	else if ( isprint(c) )      // if printable, append c to command buffer
	{
		if ( pcCmdPtr < (gacCmdMsg + CMD_MSG_SIZE) )  *pcCmdPtr++ = c;
		if ( yInteractive ) putch( c );	    // echo char back to user terminal
	}
	else if ( c == ESC || c == CAN )    // Expected from "interactive" user only
	{
		hci_clear_command();    // Trash cmd message
		hci_put_resp_term();
	}
}


/*
|   Function looks for command name (mnemonic, 2 chars) in command table;
|   if found, executes respective command function.
*/
void  hci_exec_command( void )
{
	char   c1, c2;
	uint8  n;
	bool   yFoundCndName = FALSE;

	c1 = toupper( gacCmdMsg[0] );
	c2 = toupper( gacCmdMsg[1] );

	for ( n = 0;  n < 250;  n++ )
	{
		if ( asCommand[n].cName1 == '$' )       // end of table
			break;
		if ( asCommand[n].cName1 == c1 )
		{
			if ( asCommand[n].cName2 == c2 )    // found match
			{
				yFoundCndName = TRUE;
				break;
			}
		}
	}
	if ( yFoundCndName )
	{
		if ( yInteractive )  NEW_LINE;
		(*asCommand[n].Function)();     // Do command function
	}
	else  hci_put_cmd_error();          // Unrecognised command

	hci_put_resp_term();        // Output the response terminator codes
	hci_clear_command();        // Prepare for new command
}


/*
|   Function:   hci_clear_command();
|   Clear command buffer buffer and reset pointer.
|
|   Affects:  gacCmdMsg[], pcCmdPtr, cRespCode
*/
void  hci_clear_command( void )
{
	uint8  ubx;

	for ( ubx = 0;  ubx < (CMD_MSG_SIZE+1);  ubx++ )
	{
		gacCmdMsg[ubx] = NUL;
	}
	pcCmdPtr = gacCmdMsg;
	if ( yInteractive ) cRespCode = '=';
	else  cRespCode = '-';
}


/*
|  Send response termination sequence to the HCI serial output stream.
|  In "interactive user mode", this is a prompt for new command.
*/
void  hci_put_resp_term( void )
{
	putch( '\r' );
	putch( '\n' );
	putch( cRespCode );
	if ( yInteractive ) putch( '>' );
}

/*
|  Indicate to HCI user (human or machine) that the last command msg
|  received had an error.
*/
void  hci_put_cmd_error()
{
	cRespCode = '!';   
	if ( yInteractive ) putstr("\n! Command Error");
}


/*  Dummy command function -- does nothing!  */
void  null_cmd( void )
{
	return;
}


/********************************  HOST COMMAND FUNCTIONS  ******************************/

const  char  acHelpStrDP[] PROGMEM = "DP        | Default Params\n";
const  char  acHelpStrLS[] PROGMEM = "LS        | List Command Set\n";
const  char  acHelpStrIM[] PROGMEM = "IM x      | Interactive Mode\n";
const  char  acHelpStrVN[] PROGMEM = "VN        | Show Version\n";
const  char  acHelpStrSE[] PROGMEM = "SE        | Show Errors\n";
const  char  acHelpStrSF[] PROGMEM = "SF        | Show Flags\n";
const  char  acHelpStrRS[] PROGMEM = "RS        | Reset System\n";
const  char  acHelpStrWD[] PROGMEM = "WD        | Watch Data\n";
const  char  acHelpStrDC[] PROGMEM = "DC [aaaa] | Dump Code mem\n";
const  char  acHelpStrDD[] PROGMEM = "DD [aaaa] | Dump Data mem\n";
const  char  acHelpStrDE[] PROGMEM = "DE pp     | Dump EEPROM page\n";
const  char  acHelpStrEE[] PROGMEM = "EE pp     | Erase EEPROM page\n";
const  char  acHelpStrRM[] PROGMEM = "RM aaa    | Read Memory byte\n";
const  char  acHelpStrWM[] PROGMEM = "WM aaa bb | Write Memory byte\n";
const  char  acHelpStrIR[] PROGMEM = "IP rr     | Input I/O reg\n";
const  char  acHelpStrOR[] PROGMEM = "OP rr bb  | Output I/O reg\n";

/*
|  Command function 'LS' :  Lists a command set Summary.
|  Command message format:  "LS"
*/
void  list_cmd( void )
{
	putstr_P( acHelpStrDP );
	putstr_P( acHelpStrLS );
	putstr_P( acHelpStrIM );
	putstr_P( acHelpStrVN );
	putstr_P( acHelpStrSE );
	putstr_P( acHelpStrSF );
	putstr_P( acHelpStrRS );
	putstr_P( acHelpStrWD );
	putstr_P( acHelpStrDC );
	putstr_P( acHelpStrDD );
	putstr_P( acHelpStrDE );
	putstr_P( acHelpStrEE );
	putstr_P( acHelpStrRM );
	putstr_P( acHelpStrWM );
	putstr_P( acHelpStrIR );
	putstr_P( acHelpStrOR );
}


/*
|  Command function 'IM':  Enable/disable HCI "interactive mode".
|  Cmd format: "IM x"  ... where 'x' is either '0' or '1' (alt. 'Y').
|  When interactive mode is enabled, HCI input characters are echoed back.
*/
void  interactive_cmd( void )
{
	char   c = gacCmdMsg[3];   // get the argument char

	if ( c == '1' || c == 'Y' || c == 'y' )  yInteractive = TRUE;
	else  yInteractive = FALSE;
	hci_clear_command();
}


/*
|  Command function 'WD':  Watch data memory variables, etc, in real-time.
|  Scheduled background tasks are kept alive while the Watch function executes.
|  This function is intended to be customized to suit the user application.
*/
void  watch_data_cmd( void )
{
	uint16   wStartTime = millisec_timer();
	uint16   wDelayStartTime;
	uint16   wElapsedTime;

	putstr( "Hit <Esc> to quit...\n" );

	while ( 1 )    // Loop until any key hit
	{
		wElapsedTime = millisec_timer() - wStartTime;
		// Output here data to be watched, all on a single line -------------
		// May be extended to multiple lines using terminal emulator ESC sequences.
		putDecWord( (wElapsedTime / 100), 5 );  // Output time unit = 0.1 sec
		//
		//
		putch( SPACE );     // cursor now at end of line

		wDelayStartTime = millisec_timer();     // Start 100ms delay
		while ( millisec_timer() - wDelayStartTime < 100 )
		{
			doBackgroundTasks();
		}
		putch( '\r' );      // return cursor to start of line
		if ( serialRxDataAvail() && getch() == ESC )  break;
	}
}

/*
|  Command function 'DP':  Load default configuration parameters.
|
|  Configuration parameters are application-specific persistent data maintained in EEPROM.
|  The command copies "factory default" parameter values from program code (flash memory)
|  to SRAM working variables, then invokes a task to write the data to EEPROM.
*/
void  default_params_cmd( void )
{
	// To be implemented for the user application
}


/*
|  Command function 'SE':  Show system error flags (word) then clear flags.
*/
void  show_errors_cmd( void )
{
	put_word_bits( gwSystemError );
	gwSystemError = 0;
}


/*
|  Command function 'SF':  Show system debug flags (word) then clear flags..
*/
void  show_flags_cmd( void )
{
	put_word_bits( gwDebugFlags );
	gwDebugFlags = 0;
}


/*
|  Command function 'VN':  Print firmware version number & build date/time.
|
|  Response format:  "Vm.n.ddd : MJB Aug 02 2011" (example)
*/
void  version_cmd( void )
{
	putch( 'V' );
	putDecWord( BUILD_VER_MAJOR, 1 );
	putch( '.' );
	putDecWord( BUILD_VER_MINOR, 1 );
	putch( '.' );
	putDecWord( BUILD_VER_DEBUG, 3 );
	if ( yInteractive ) 
	{
		putstr( " MJB " );
		putstr( (char *) __DATE__ );   
		if ( yInteractive ) NEW_LINE;
	}
}


/*
|  Command function 'RS':  Reset MCU / restart program.
|  If watchodog enabled, turns off interrupts and forces watchdog reset;
|  otherwise, the function simply jumps to address 0x0000.
*/
void  reset_MCU_cmd( void )
{
	DISABLE_GLOBAL_IRQ;

#ifdef  WATCHDOG_SUPPORTED
	while ( 1 )  continue;
#else
	asm(" JMP 0x0000 ");
#endif
}


/*
|  Command function 'Dx':  Dumps a 256 byte block of memory in Hex ASCII format.
|
|  The command mnemonic may be 'DC', 'DD' or 'DE'.
|  If it is 'DC', the program code (flash) memory space is accessed;
|  if it is 'DE', the EEPROM space is mapped in; the dump block size is ??? bytes;
|  if it is 'DD', the SRAM data space is accessed.
|
|  In the case of 'DE', the command argument is an EEPROM page number (00..FF);
|  otherwise the argument is a hexadecimal address (optional).
|  If no address is given, the previous value is used, incremented by 256.
|  The dump begins on a 16 byte boundary ($aaa0), regardless of the argument LSD.
|
|  Arg1 @ CmdMsg[3] is start addr (0..FFFF) (optional), or EEPROM page (00..FF)
*/
void  dump_memory_cmd( void )
{
	static  uint16  uwStartAddr;    // remembered for next time command used
	uint16  uwAddr, uwArgValue;
	uint8   ubRow, ubCol, ubDat;
	char    c2;
	uint8   ubPageRows = 16;

	c2 = toupper( gacCmdMsg[1] );
	uwArgValue = hexatoi( &gacCmdMsg[3] );      // defaults to 0 if invalid arg.

	if ( c2 == 'E' )     // Assume EEPROM page # given
	{
		uwAddr = (uwArgValue & 7) * 128;
		ubPageRows = 8;
		hci_put_cmd_error();    // TEMP: until eeprom_read_byte() is implemented
		return;
	}
	else if ( isHexDigit( gacCmdMsg[3] ) )      // Start address given...
	{
		uwStartAddr = uwArgValue & 0xFFF0;       // ... save it for next time
		uwAddr = uwStartAddr;
	}
	else  uwAddr = uwStartAddr;    // No arg given -- use last value

	for ( ubRow = 0;  ubRow < ubPageRows;  ubRow++ )
	{
		putHexWord( uwAddr );
		putch( SPACE );
		for ( ubCol = 0;  ubCol < 16;  ubCol++ )
		{
			putch( SPACE );
			if ( ubCol == 8 )  putch( SPACE );
			if ( c2 == 'E' )  ubDat = eeprom_read_byte( uwAddr );
			else if ( c2 == 'C' )  ubDat = pgm_read_byte( uwAddr );
			else  ubDat = *(uint8 *)( uwAddr );
			putHexByte( ubDat );
			uwAddr++ ;
		}
		putch( SPACE );
		putch( SPACE );
		uwAddr -= 16;
		for ( ubCol = 0;  ubCol < 16;  ubCol++ )
		{
			if ( c2 == 'E' )  ubDat = eeprom_read_byte( uwAddr );
			else if ( c2 == 'C' )  ubDat = pgm_read_byte( uwAddr );
			else  ubDat = *(uint8 *)( uwAddr );
			if ( ubDat >= 32 && ubDat < 127 )  putch( ubDat );
			else  putch( SPACE );
			uwAddr++ ;
		}
		NEW_LINE;
	}
	if ( c2 != 'E' )  uwStartAddr += 256;    // Show next 256-byte block next time
}


/*
|  Command function 'RM': Read and output a data memory (SRAM) byte value (2 hex).
|  This command may also be used to access MCU registers and I/O registers.
|
|  Arg1 @ CmdMsg[3] is a 3-digit hex address in data memory space (000..FFF)
*/
void  read_data_mem_cmd( void )
{
	if ( yInteractive ) putch( SPACE );
	putHexByte( *(uint8 *) hexatoi( &gacCmdMsg[3] ) );
}


/*
|  Command function 'WM':  Write byte value (hex) to data memory (SRAM) address.
|  This command may also be used to access MCU registers and I/O registers.
|
|  Arg1 @ CmdMsg[3] is 3-digit memory address in data space (000..FFF)
|  Arg2 @ CmdMsg[7] is 2-digit data (byte) value to write.
|  The write is not verified.
*/
void  write_data_mem_cmd( void )
{
	if ( !isHexDigit( gacCmdMsg[3] )        // Syntax check
	||   !isHexDigit( gacCmdMsg[4] )
	||   !isHexDigit( gacCmdMsg[5] )
	||   gacCmdMsg[6] != SPACE
	||   !isHexDigit( gacCmdMsg[7] )
	||   !isHexDigit( gacCmdMsg[8] ) )
	{
		hci_put_cmd_error();    // Syntax error
	}
	else
	{
		*(uint8 *) hexatoi( &gacCmdMsg[3] ) = (uint8)hexatoi( &gacCmdMsg[7] );
	}
}


/*
|   Command function 'IP':  Input and show byte value (hex) of an I/O register.
|   The specified address is assumed to be in the I/O register space (00..3F).
|   The function adds 0x20 to the address so as to access the register in data
|   memory space.
|
|   Arg1 @ CmdMsg[3] is I/O register addr as 2-digit hex (00..3F)
*/
void  input_IOreg_cmd( void )
{
	if ( yInteractive ) putch( SPACE );
	putHexByte( *(uint8 *) ( hexatoi( &gacCmdMsg[3] ) + 0x20 ) );
}


/*
|   Command function 'OP':  Output a byte value (hex) to an I/O register.
|   The specified address is assumed to be in the I/O register space (00..3F).
|   The function adds 0x20 to the address so as to access the register in data
|   memory space.
|
|   Arg1 @ CmdMsg[3] is I/O register addr as 2-digit hex (00..3F)
|   Arg2 @ CmdMsg[6] is the (hex) value to be written.
*/
void  output_IOreg_cmd( void )
{
	if ( !isHexDigit( gacCmdMsg[3] )        // Syntax check
	||   !isHexDigit( gacCmdMsg[4] )
	||   gacCmdMsg[5] != SPACE
	||   !isHexDigit( gacCmdMsg[6] )
	||   !isHexDigit( gacCmdMsg[7] ) )
	{
		hci_put_cmd_error();    // Syntax error
	}
	else
	{
		*(uint8 *) (hexatoi( &gacCmdMsg[3] ) + 0x20 ) = (uint8) hexatoi( &gacCmdMsg[6] );
	}
}


/*
|  TODO: Command function 'EE':  Erase specified EEPROM page.
|
|  The specified EEPROM page (??? bytes) is filled with 0xFF.
|  Background tasks are delayed until the command is finished.
*/
void  erase_eeprom_cmd( void )
{
	cRespCode = '!';
}


/******************************  HCI "I/O LIBRARY" FUNCTIONS  ***************************/

/*
|   getchar() - Input next available character from serial RX FIFO buffer.
|
|   Unlike getch(),  getchar() waits for data available in the RX FIFO buffer,
|   then reads the next unread char, echoes it back to the serial output stream,
|   and returns with it.
|
|   While waiting for input, the background task dispatcher is called,
|   so as not to delay any pending or currently executing task.
|   To avoid the possibility of infinite recursion, this function cannot be
|   called from any scheduled background task.
|
|   Returns:  (uchar) byte from serial RX buffer.
*/
uchar  getchar( void )
{
	char   c;

	while ( !serialRxDataAvail() )
	{
		doBackgroundTasks();
	}
	putch ( c = getch() );
	return ( c );
}

/*
|  Output a NUL-terminated string to the HCI serial port.
|  The string is expected to be in the data memory (SRAM) space.
|  Newline (0x0A) is expanded to CR + LF (0x0D + 0x0A).
|
|  After outputting the string, the background task dispatcher is called,
|  so as to minimize the delay of any pending or currently executing task.
|  To avoid the possibility of infinite recursion, this function cannot be
|  called from any scheduled background task.
|
|  Called by:   HCI command functions only (NOT scheduled B/G tasks!)
|  Entry args:  pstr = address of NUL-terminated string in SRAM.
*/
void  putstr( char * pstr )
{
	char   c;

	while ( (c = *pstr++) != NUL )
	{
		if ( c == '\n' )
		{
			putch( '\r' );
			putch( '\n' );
		}
		else   putch( c );
	}
	doBackgroundTasks();
}

/*
|  Output a NUL-terminated string to the HCI serial port.
|  The string is expected to be in the program code (flash) memory space.
|  Newline (0x0A) is expanded to CR + LF (0x0D + 0x0A).
|
|  After outputting the string, the background task dispatcher is called,
|  so as to minimize the delay of any pending or currently executing task.
|  To avoid the possibility of infinite recursion, this function cannot be
|  called from any scheduled background task.
|
|  Called by:   HCI command functions only (NOT scheduled B/G tasks!)
|  Entry args:  pksz = address of NUL-terminated string in PROGMEM.
*/
void  putstr_P( PGM_P pksz )
{
	char   c;

	while ( (c = pgm_read_byte(pksz)) != NUL )
	{
		if ( c == '\n' )
		{
			putch( '\r' );
			putch( '\n' );
		}
		else   putch( c );
		pksz++ ;
	}
	doBackgroundTasks();
}

/*
|  Output Boolean value as ASCII '0' or '1'.
|
|  Called by:  command functions, etc.
|  Entry args: b = Boolean variable (zero or non-zero)
|  Returns:    void
|  Affects:
*/
void  putBoolean( bool  b )
{
	if ( b )  putch( '1');
	else  putch ( '0' );
}

/*
|  Output 4 LS bits of a byte as Hex (or BCD) ASCII char.
|
|  Called by:  command functions, etc.
|  Entry args: d = value of Hex digit (0 to 0xf)
|  Returns:    void
|  Affects:    --
*/
void  putHexDigit( uint8 d )
{
	d &= 0x0F;
	if ( d < 10 )  putch ( '0' + d );
	else  putch ( 'A' + d - 10 );
}

/*
|  Output byte as 2 Hex ASCII chars, MSD first.
|
|  Called by:  command functions, etc.
|  Entry args: b = byte to output
|  Returns:    void
|  Affects:    --
*/
void  putHexByte( uint8 b )
{
	putHexDigit( b >> 4 );
	putHexDigit( b );
}

/*
|  Output 16-bit word as 4 Hex ASCII chars, MSD first.
|
|  Called by:  command functions, etc.
|  Entry args: uwArg1 = word to output
|  Returns:    void
|  Affects:    --
*/
void  putHexWord( uint16 uwArg1 )
{
	putHexDigit( (uint8) (uwArg1 >> 12) );
	putHexDigit( (uint8) (uwArg1 >> 8) );
	putHexDigit( (uint8) (uwArg1 >> 4) );
	putHexDigit( (uint8) (uwArg1 & 0xF) );
}

/*
|  Output a 16-bit unsigned word as an ASCII decimal number, with leading zeros.
|  The number of digit places to be output (0 to 5) is specified as a parameter.
|  If the decimal word value is larger than can fit into the number of places
|  specified, then the output will be truncated to the least significant digit(s).
|  If the decimal word value is smaller than can occupy the number of places
|  specified, then the output will be padded with leading 0's.
|
|
|  Called by:  command functions, etc
|  Entry args: (uint16) uwArg1 = word to output
|              (uint8) ubPlaces = number of digit places to output (1..5)
|  Returns:    void
|  Affects:    --
*/
void  putDecWord( uint16 uwArg1, uint8 ubPlaces )
{
	int8   bPos;
	uint8  aubDigit[5];    // BCD result, 1 byte for each digit

	if ( ubPlaces > 5 )  ubPlaces = 5;
	for ( bPos = 4;  bPos >= 0;  --bPos )
	{
		aubDigit[bPos] = uwArg1 % 10;
		uwArg1 /= 10;
	}
	for ( bPos = 5 - ubPlaces;  bPos < 5;  ++bPos )
		putHexDigit( aubDigit[bPos] );
}

/*
|  Output 16-bit word as 16 binary digits, MS bit first.
|
|  Called by:  command functions, etc.
|  Entry args: uwArg1 = word to output
*/
void  put_word_bits( uint16 wArg )
{
	uint16   wBit;

	for ( wBit = 0x8000;  wBit != 0;  wBit >>= 1 )
	{
		putch( (wArg & wBit) ? '1' : '0' );
		putch( SPACE );
		if ( wBit == 0x0100 ) { putch( SPACE ); putch( SPACE ); }
	}
}


/*****************************************************************************************
|                           CHARACTER CONVERSION FUNCTIONS
|
|  Convert decimal ASCII char to 4-bit BCD value (returned as unsigned byte).
|
|  Entry args: c = decimal digit ASCII encoded
|  Returns:    0xFF if arg is not a decimal digit, else unsigned byte (0..9)
|  Affects:    ---
*/
uint8  dectobin( char c )
{
	if ( c >= '0'  &&  c <= '9')
		return ( c - '0' );
	else
		return 0xFF ;
}


/*
|  Convert decimal ASCII string, up to 5 digits, to 16-bit unsigned word.
|  There may be leading zeros, but there cannot be any leading white space.
|  Conversion is terminated when a non-decimal char is found, or when the
|  specified number of characters has been processed.
|
|  Entry args: (char *) pac = pointer to first char of decimal ASCII string
|              (int8)  bNdigs = number of characters to process (max. 5)
|  Returns:    Unsigned 16bit word ( 0 to 0xffff ).
|              If the target string (1st char) is non-numeric, returns 0.
*/
uint16  decatoi( char * pnac, int8 bNdigs )
{
	uint8   ubDigit, ubCount;
	uint16  uwResult = 0;

	for ( ubCount = 0;  ubCount < bNdigs;  ubCount++ )
	{
		if ( (ubDigit = dectobin( *pnac++ )) == 0xFF )
			break;
		uwResult = 10 * uwResult + ubDigit;
	}
	return  uwResult;
}


/*
|  Convert Hexadecimal ASCII char (arg) to 4-bit value (returned as unsigned byte).
|
|  Called by:  various, background only
|  Entry args: c = Hex ASCII character
|  Returns:    0xFF if arg is not hex, else digit value as unsigned byte ( 0..0x0F )
*/
uint8  hexctobin( char c )
{
	if ( c >= '0'  &&  c <= '9')
		return ( c - '0' );
	else if ( c >= 'A'  &&  c <= 'F' )
		return ( c - 'A' + 10 );
	else if ( c >= 'a'  &&  c <= 'f' )
		return ( c - 'a' + 10 );
	else
		return 0xFF ;
}


/*
|  Convert Hexadecimal ASCII string, up to 4 digits, to 16-bit unsigned word.
|  The string must be stored in the data RAM space.
|  There cannot be any leading white space.
|  Conversion is terminated when a non-Hex char is found.
|
|  Entry args: (char *) s = pointer to first char of hex string.
|  Returns:    Unsigned 16bit word ( 0 to 0xffff ).
|              If the target string (1st char) is non-Hex, returns 0.
*/
uint16  hexatoi( char * s )
{
	uint8   ubDigit, ubCount;
	uint16  uwResult = 0;

	for ( ubCount = 0;  ubCount < 4;  ubCount++ )
	{
		if ( (ubDigit = hexctobin( *s++ )) == 0xFF )
			break;
		uwResult = 16 * uwResult + ubDigit;
	}
	return  uwResult;
}


/*
|  Function returns TRUE if char is hex ASCII digit ('0'..'F')
*/
bool  isHexDigit( char c )
{
	if ( hexctobin( c ) == 0xFF ) return FALSE;
	else  return TRUE;
}


// end
