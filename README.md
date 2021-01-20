## AVRMonitor
AVRMonitor is a monitor application to access memory and IO space for AVR ATMEGA Microcontroller from a series of commands over a serial port. It was developed by Michael J Bauer. A very good AVR C tutorial and series of AVR resources are available from Michael at http://www.mjbauer.biz/mjb_resources.htm. 

This version is for the ATMEGA328P as used on the Arduino UNO board. 
The changes relate to setting up the serial port and IO usage for the Arduino. The device is programmed through the Arduino inbuild bootloader use AVRDude on the host PC. The monitor command interface is through the AVR serial port. This is accessed through the Arduino USB interface as a virtual serial port on the host machine. 

There is a simple task scheduler to run routines on a periodic basis. All tasks run within one memory space.  
## Monitor Commands
 * DP        | Default Params
 * LS        | List Command Set
 * IM x      | Interactive Mode
 * VN        | Show Version
 * SE        | Show Errors
 * SF        | Show Flags
 * RS        | Reset System
 * WD        | Watch Data
 * DC [aaaa] | Dump Code mem
 * DD [aaaa] | Dump Data mem
 * DE pp     | Dump EEPROM page
 * EE pp     | Erase EEPROM page
 * RM aaa    | Read Memory byte
 * WM aaa bb | Write Memory byte
 * IP rr     | Input I/O reg
 * OP rr bb  | Output I/O reg
## IO used
* Port C bits 0:5 are each connected to a led which is connected via a 300R resistor to 5V. These are used by a demo background task to chase a pattern on the leds.
* Port B bit 0 is connected to single led connected to 300R resistor to 5V. This provides for 1 sec heartbeat.
* Serial port is set up as 19200 baud, 8 data bits, no parity and no stop bits.
## Task Scheduler
The task scheduler provide for tasks to be executed as

* 5mSec periodic task 
* 50mSec periodic task 
* 500mSec periodic task

## Build Environment

Microchip Studio 7 Version: 7.0

avr-gcc (AVR_8_bit_GNU_Toolchain_3.6.2_1778) 5.4.0

Microsoft Windows 10


