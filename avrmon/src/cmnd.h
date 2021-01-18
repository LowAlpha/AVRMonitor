/*
 **********************************************************************************
 *
 *  Project Name:  "AVROS" AVR operating system for ATmega micro-controller
 *
 *  File Name:  fnproto.h
 *
 *  Copyright 2008 by M.J.Bauer  [www.hotkey.net.au/~mjbauer]
 *
 *  Date Created: 2008.02.03
 *
 **********************************************************************************
 */
#ifndef  FNPROTO_H_
#define  FNPROTO_H_

#define  CMD_MSG_SIZE      (63)     // Maximum command string length

#define  NEW_LINE          { putch('\r'); putch('\n'); }


/*_______________________  F U N C T I O N   P R O T O T Y P E S  ______________________*/

void   hci_init(void);                              // initialises HCI variables
void   hci_service( void );                     // checks for data received from HCI stream
void   hci_process_input( char c );             // builds a command message
void   hci_exec_command( void );                // executes host command
void   hci_clear_command( void );               // clears command msg buffer; resets pointer
void   hci_put_resp_term( void );               // Outputs the termination (prompt) chars
void   hci_put_cmd_error(void);                     // Outputs "! Command Error" (interactive only)

void   null_cmd( void );                        // Command functions
void   list_cmd( void );                        
void   interactive_cmd( void );
void   set_date_cmd( void );
void   set_time_cmd( void );
void   version_cmd( void );
void   watch_data_cmd( void );
void   default_params_cmd( void );
void   show_errors_cmd( void );
void   show_flags_cmd( void );
void   reset_MCU_cmd( void );
void   dump_memory_cmd( void );
void   read_data_mem_cmd( void );
void   write_data_mem_cmd( void );
void   input_IOreg_cmd( void );
void   output_IOreg_cmd( void );
void   erase_eeprom_cmd( void );

uchar  getchar( void );
void   putstr( char * );                        // output string, NUL terminated
void   putstr_P( PGM_P pks );                   // output PROGMEM string, NUL term.
void   putBoolean( bool );                      // output Boolean value as '0' or '1'
void   putHexDigit( uint8 );                    // output LS nybble as hex ASCII char
void   putHexByte( uint8 );                     // output byte as 2 Hex ASCII chars
void   putHexWord( uint16 );                    // output word as 4 Hex ASCII chars
void   putDecWord( uint16, uint8 );             // output word as 1..5 decimal ASCII
void   put_word_bits( uint16 wArg );            // output word as 16 binary digits

uint8  dectobin( char c );                      // convert dec ASCII digit to binary
uint16 decatoi( char * pnac, int8 bNdigs );     // convert dec ASCII string to integer
uint8  hexctobin( char c );                     // convert hex ASCII digit to binary
uint16 hexatoi( char * s );                     // convert hex ASCII string to integer
bool   isHexDigit( char c );                    // Rtn TRUE if char is hex ASCII digit

#endif  // FNPROTO_H_
