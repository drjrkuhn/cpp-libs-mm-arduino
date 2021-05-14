/**
\ingroup	ArduinoCommon
\file		UsbDebugPrint.h
\brief		Extra debugging information for USB Host Shield Library
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: UsbDebugPrint.h 437 2016-01-25 00:28:13Z jkuhn $
$Author: jkuhn $
$Revision: 437 $
$Date: 2016-01-24 18:28:13 -0600 (Sun, 24 Jan 2016) $

*/

#pragma once

#include "DebugPrint.h"
#include <UsbCore.h>

//////////////////////////////////////////////////////////
/// \name Error Printing Macros
/// \ingroup ArduinoCommon
///@{

/** \ingroup ArduinoCommon

helper macro for printing case constants. See printUsbTaskStateStr for an example 

*/
#define PRINT_CONST_CASE(out,def) case def: out.print(F(#def)); break;

/** \ingroup ArduinoCommon

helper macro for printing default value. See printUsbTaskStateStr for an example 

*/
#define PRINT_CONST_DEFAULT(out,T,var)	default: out.print(F("unknown " #var ": ")); printHex<T>(var);

///@}
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
/// \name USB Host Error Codes
/// \ingroup ArduinoCommon
/// Extra USB Host error return codes. 
///
/// Uses the space from 0xA0 to 0xBF to keep from 
/// conflicts with max3421e.h and UsbCore.h
///@{

#define hrBADBUFFER				0xA0		///< Buffer was too small
#define hrTERMNOTFOUND			0xA1		///< Termination string/character was not found
#define hrBADREPLY				0xA2		///< received a bad reply from the BrimAOTF
#define hrNULLRCVD				0xA3		///< NULL character '\0' received from AOTF. This appears to precede an error
#define hrNOTSTARTED			0xA4		///< The device has not been started yet.
#define hrINTERNALERROR			0xAF		///< Problem with an internal algorithm

///@}
//////////////////////////////////////////////////////////


/** \ingroup ArduinoCommon

DebugPrint helper sub-class provides debugging messages
specific to the USB Host Library.

\note These derived methods cannot be chained to the end
of a DebugPrint& return value. Instead, just start a
new method chain.

\code{.cpp}
	UsbDebugPrint out(Serial);
	out.print_P(PSTR("Hello, world").println();		// legal
	//out.println().printUsbTaskStateStr(0x10).println();	// ILLEGAL
	out.printUsbTaskStateStr(0x10).println();				// legal
\endcode
*/
class UsbDebugPrint : public DebugPrint {
public:

	//////////////////////////////////////////////////////////
	/// \name constructor
	///
	/// The default constructor takes an output Stream reference.
	/// It defaults to no endlines at the end of a message and
	/// messages continuously print to the the stream without flushing.
	///@{

	/** Constructor */
	UsbDebugPrint(Stream& __out) 
		: DebugPrint(__out) {}

	///@}
	//////////////////////////////////////////////////////////

	/** 
	Print a string representing a Usb::getTaskState() return value.

	Task states such as "USB_STATE_DETACHED" are defined in <UsbCore.H>. 
	*/
	// in INSTANTIATE
	UsbDebugPrint& printUsbTaskStateStr(const uint8_t __state);

	/** 
	Print a string representing a MAX3421e USB Host error result codes. 

	Error codes such as "bmRCVTOGRD" are defined in <max3421e.h> 
	*/
	// in INSTANTIATE
	UsbDebugPrint& printHostErrorStr(const uint8_t __rcode);

	/** Print an error message and a HostError return code as text */
	template<typename T>
	UsbDebugPrint& logHostErrorMsg(T __t, uint8_t __rcode) {
		startLogError().print(__t).print_P(msgSep_P);
		printHostErrorStr(__rcode).endLog();
		return *this;
	}

	/** Print an error message from PROGMEM and a HostError return code as text */
	UsbDebugPrint& logHostErrorMsg_P(PGM_P __msg_P, uint8_t __rcode) {
		startLogError().print_P(__msg_P).print_P(msgSep_P);
		printHostErrorStr(__rcode).endLog();
		return *this;
	}

	/** Print an error message and a buffer as ascii text */
	template<typename T>
	UsbDebugPrint& logBufferErrorMsg(T __t, const uint8_t* __buf, size_t __size, bool __fixedWidth=false)	{
		startLogError().print(__t).print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth);
		endLog();
		return *this;
	}

	/** Print an error message from PROGMEM and a buffer as ascii text */
	UsbDebugPrint& logBufferErrorMsg_P(PGM_P __msg_P, const uint8_t* __buf, size_t __size, bool __fixedWidth=false)	{
		startLogError().print_P(__msg_P).print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth);
		endLog();
		return *this;
	}

protected:
};

/////////////////////////////////////////////////////////////////////////////
/// Instantiate one definition rules. 
/// @see ArduinoCommon for explanation.
/////////////////////////////////////////////////////////////////////////////
#if defined(INSTANTIATE_USBDEBUGPRINT_CPP) || defined(INSTANTIATE_ALL) || defined(__INTELLISENSE__)

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Usb.h>

UsbDebugPrint&  UsbDebugPrint::printUsbTaskStateStr(const uint8_t state)
{
	switch (state) {
		PRINT_CONST_CASE(out_, USB_STATE_DETACHED);
		PRINT_CONST_CASE(out_, USB_DETACHED_SUBSTATE_INITIALIZE);
		PRINT_CONST_CASE(out_, USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE);
		PRINT_CONST_CASE(out_, USB_DETACHED_SUBSTATE_ILLEGAL);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_SETTLE);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_RESET_DEVICE);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_WAIT_SOF);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_WAIT_RESET);
		PRINT_CONST_CASE(out_, USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE);
		PRINT_CONST_CASE(out_, USB_STATE_ADDRESSING);
		PRINT_CONST_CASE(out_, USB_STATE_CONFIGURING);
		PRINT_CONST_CASE(out_, USB_STATE_RUNNING);
		PRINT_CONST_CASE(out_, USB_STATE_ERROR);
		PRINT_CONST_DEFAULT(out_, uint8_t, state);
	}
	return *this;
}

UsbDebugPrint& UsbDebugPrint::printHostErrorStr(const uint8_t rcode)
{
	switch (rcode) {
		PRINT_CONST_CASE(out_, hrSUCCESS);
		PRINT_CONST_CASE(out_, hrBUSY);
		PRINT_CONST_CASE(out_, hrBADREQ);
		PRINT_CONST_CASE(out_, hrUNDEF);
		PRINT_CONST_CASE(out_, hrNAK);
		PRINT_CONST_CASE(out_, hrSTALL);
		PRINT_CONST_CASE(out_, hrTOGERR);
		PRINT_CONST_CASE(out_, hrWRONGPID);
		PRINT_CONST_CASE(out_, hrBADBC);
		PRINT_CONST_CASE(out_, hrPIDERR);
		PRINT_CONST_CASE(out_, hrPKTERR);
		PRINT_CONST_CASE(out_, hrCRCERR);
		PRINT_CONST_CASE(out_, hrKERR);
		PRINT_CONST_CASE(out_, hrJERR);
		PRINT_CONST_CASE(out_, hrTIMEOUT);
		PRINT_CONST_CASE(out_, hrBABBLE);
		// Extra error codes
		PRINT_CONST_CASE(out_, hrBADBUFFER);
		PRINT_CONST_CASE(out_, hrTERMNOTFOUND);
		PRINT_CONST_CASE(out_, hrBADREPLY);
		PRINT_CONST_CASE(out_, hrNULLRCVD);
		PRINT_CONST_CASE(out_, hrNOTSTARTED);
		PRINT_CONST_CASE(out_, hrINTERNALERROR);
		PRINT_CONST_DEFAULT(out_, uint8_t, rcode);
	}
	return *this;
}

#endif // #ifdef INSTANTIATE_USBDEBUGPRINT_CPP

//////////////////////////////////////////////////////////
/// Test Code
//////////////////////////////////////////////////////////
#if defined(TEST_USBDEBUGPRINT) || defined(__INTELLISENSE__)

#include <Arduino.h>
#include <HardwareSerial.h>

#define BUFFER_WITH_CODES "\x02" "Ascii buffer\t" "\x07" "\r\nseveral\0" "\x7f" "\x7c" "codes.\xAB\xCD\xEF\x04"
const char testBufferChar[] = BUFFER_WITH_CODES;
const uint8_t* testBuffer = reinterpret_cast<const uint8_t*>(testBufferChar);
const size_t testLen = sizeof(BUFFER_WITH_CODES) - 1;
const char testBuffer_P[] PROGMEM = BUFFER_WITH_CODES;

#define INT_MSG		12345
#define CHAR_MSG	"const char* message"
#define STRING_MSG	String("String message")
#define F_MSG		F("F message")
#define PSTR_MSG	PSTR("PSTR message")

extern "C" {
	void setup() {
		Serial.begin(115200);
		delay(500);
		while (!Serial) {
			// wait for serial to connect
		}
		UsbDebugPrint out(Serial);
		out.logEndl();

		out.endl().print(F("Test printTaskState")).endl();
		out.printRepeat("=", 40).endl();

		const uint8_t states[] = { 0x10, 0x11, 0x12, 0x13, 0x20, 0x30, 0x40, 0x50, 0x51, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xff };
		const size_t len = sizeof(states) / sizeof(uint8_t);
		out.print(F("index\tstate\tstring\n"));
		for (int i = 0; i < len; i++) {
			out.print(i).print("\t").printHex(states[i]).print("\t");
			out.printUsbTaskStateStr(states[i]).endl();
		}
		out.endl();

		out.endl().print(F("Test printHostErrorStr")).endl();
		out.printRepeat("=", 40).endl();

		out.print(F("state\tstring\n"));
		for (int i = 0; i <= 0x10; i++) {
			out.printHex<uint8_t>(i).print("\t");
			out.printHostErrorStr(i).endl();
		}
		int ts = USB_STATE_DETACHED, hr = hrBABBLE;

		out.endl().print(F("Test logHostErrorMsg")).endl();
		out.printRepeat("=", 40).endl();

		out.logHostErrorMsg(INT_MSG, hr);
		out.logHostErrorMsg(CHAR_MSG, hr);
		out.logHostErrorMsg(STRING_MSG, hr);
		out.logHostErrorMsg(F_MSG, hr);
		out.logHostErrorMsg_P(PSTR_MSG, hr);

		bool fixedWidth = false;
		out.endl().print(F("Test logHostErrorMsg fixedWidth=")).print(fixedWidth).endl();
		out.printRepeat("=", 40).endl();

		out.logBufferErrorMsg(INT_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(CHAR_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(STRING_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(F_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg_P(PSTR_MSG, testBuffer, testLen, fixedWidth);

		fixedWidth = true;
		out.endl().print(F("Test logHostErrorMsg fixedWidth=")).print(fixedWidth).endl();
		out.printRepeat("=", 40).endl();

		out.logBufferErrorMsg(INT_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(CHAR_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(STRING_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg(F_MSG, testBuffer, testLen, fixedWidth);
		out.logBufferErrorMsg_P(PSTR_MSG, testBuffer, testLen, fixedWidth);
	}

	void loop() {
		
	}
}

#endif
