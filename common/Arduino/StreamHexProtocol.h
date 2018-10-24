/**
\defgroup	StreamHexProtocol	Hexadecimal Protocol, Stream (Slave)

Implements HexProtocolBase on the Arduino side.
*/

/**
\ingroup StreamHexProtocol

\page ExampleStreamHexProtocol StreamHexProtocol Example

StreamHexProtocol implements a HexProtocolBase on the slave (Arduino/Genuino)
side. It uses a pointer to a Stream object for reading and writing. Most likely, 
the Stream* will point to the global Serial object defined in "HardwareSerial.h". 

The easiest way to use the StreamHexProtocol is by deriving a Handler class from it.

\code{.cpp}
class MyHandler : public StreamHexProtocol<MyHandler> {
	void doProcessCommand(prot_cmd_t __cmd)	{
		switch (__cmd) {
		case GET_VALUE:
			processGet<uint16_t>(__cmd, &MyHandler::doGetValue);
			break;
		case SET_VALUE:
			processGet<uint16_t>(__cmd, &MyHandler::doSetValue);
			break;
		default:
			replyError();
	}

	bool doGetValue(uint16_t& __val) {
		__val = value;
		return true;
	}

	bool doSetValue(const uint16_t __val) {
		value = __val;
		// do something with the value
		return true;
	}

protected:
	uint16_t value;
};

MyHandler handler;

void setup()
{
	// Higher speeds do not appear to be reliable
	Serial.begin(BAUDRATE);
	Serial.setTimeout(TIMEOUT);
    handler.startProtocol(&handler, &Serial);
}

void loop()
{
	if (handler.hasCommand()) {
		handler.processCommand(handler.getCommand(), &CHandler::doProcessCommand);
	}
}
\endcode

*/

/**
\ingroup	StreamHexProtocol
\file		StreamHexProtocol.h
\brief		Standard Arduino-side HexProtocol implementation
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: StreamHexProtocol.h 450 2016-01-29 22:57:23Z jkuhn $
$Author: jkuhn $
$Revision: 450 $
$Date: 2016-01-29 16:57:23 -0600 (Fri, 29 Jan 2016) $

 */

#pragma once

#include "..\HexProtocol.h"
#include "Stream.h"

/** \ingroup StreamHexProtocol
	Arduino pin that goes high during Serial sends or receives. Set to 0 for no diagnostic pin */
#define HEXPROT_SNDRCV_PIN		49

#if (HEXPROT_SNDRCV_PIN > 0)
#define SETUP_SNDRCV_PIN	pinMode(HEXPROT_SNDRCV_PIN, OUTPUT)
#define BEGIN_SNDRCV_PIN	digitalWrite(HEXPROT_SNDRCV_PIN, HIGH)
#define END_SNDRCV_PIN		digitalWrite(HEXPROT_SNDRCV_PIN, LOW)
#else
#define SETUP_SNDRCV_PIN
#define BEGIN_SNDRCV_PIN
#define END_SNDRCV_PIN
#endif

namespace hprot {

	/** \ingroup StreamHexProtocol 
		The Arduino library Stream class is the base for character and binary based streams. 
	*/
	typedef Stream* STREAM_T;

	/**
		Implements HexProtocolBase on the Arduino side.
		\ingroup StreamHexProtocol
	*/
	template <class DEV>
	class StreamHexProtocol : public HexProtocolBase<DEV, STREAM_T> {
	public:

		StreamHexProtocol() {
			SETUP_SNDRCV_PIN;
		}

		virtual ~StreamHexProtocol() {}

	protected:
		typedef HexProtocolBase<DEV, STREAM_T> BaseClass;

		/////////////////////////////////////////////////////////////////////////
		/// \name HexProtocolBase Implementation
		///
		///@{

		/** Write a single byte to the output stream. */
		bool writeByte(prot_byte_t b) override {
			if (!BaseClass::hasStarted()) {
				return false;
			}
			BEGIN_SNDRCV_PIN;
			size_t nbytes = BaseClass::stream_->write(static_cast<uint8_t>(b));
			END_SNDRCV_PIN;
			return (nbytes == 1);
		}

		/** Write several bytes to the output stream. The term character should
		be included in the buffer, or you can use a single writeByte()
		to write the term character. */
		size_t writeBuffer(const char* buffer, size_t size) override {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
			BEGIN_SNDRCV_PIN;
			size_t nbytes = BaseClass::stream_->write(buffer, size);
			END_SNDRCV_PIN;
			return nbytes;
		}

		/** Read a string of bytes from the stream **UNTIL** a terminator character is received, or a
		timeout occurrs. The terminator character is NOT added to the end of the buffer. */
		size_t readBufferUntilTerminator(char* buffer, size_t size, char terminator) override {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
			// NOTE: readBytesUntil does not store the terminator character
			BEGIN_SNDRCV_PIN;
			size_t nbytes = BaseClass::stream_->readBytesUntil(terminator, buffer, size);
			END_SNDRCV_PIN;
			return nbytes;
		}

		/** Reads a string of arbitrary length from the stream **UNTIL** a terminator character is
		received, or a timeout occurrs. The terminator character is NOT added to the end
		of the string, but the string is null terminated.
		\note On the Arduino, readStringUnreadStringUntilTerminatortil is much slower than
		readBufferUntilTerminator because each additional character might trigger a reallocation
		to increase the string buffer size. readStringUntilTerminator is primarily used for
		getValue<prot_string_t>(). */
		size_t readStringUntilTerminator(prot_string_t& str, char terminator) override {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
			// NOTE: readBytesUntil does not store the terminator character
			BEGIN_SNDRCV_PIN;
			str = BaseClass::stream_->readStringUntil(terminator);
			size_t len = str.length();
			END_SNDRCV_PIN;
			return len;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name HexProtocolBase Slave Implementation Methods
		/// Only the slave driver(Arduino) must implement these virtual methods.
		/// The master (PC) does not receive single byte commands. It only sends them.
		///
		///@{

		/** Check to see if the input stream has a byte to read. */
		bool hasByte() override {
			if (!BaseClass::hasStarted()) {
				return false;
			}
			return (BaseClass::stream_->available() > 0);
		}

		/** Read a single byte from the input stream. */
		bool readByte(prot_byte_t& b) override {
			if (!BaseClass::hasStarted()) {
				return false;
			}
			BEGIN_SNDRCV_PIN;
			int i = BaseClass::stream_->read();
			END_SNDRCV_PIN;
			if (i != -1) {
				b = static_cast<prot_byte_t>(i);
				return true;
			}
			return false;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Sending strings from flash memory, Low-level
		///
		///@{

#ifdef PROGMEM
		/** Send a string from program space. */
		bool putString_P(PGM_P __str_P) {
			size_t len = strlen_P(__str_P);
			size_t bytesWritten = 0;
			prot_byte_t ch;
			while (ch =  pgm_read_byte(__str_P++)) {
				if (!writeByte(ch)) {
					break;
				}
				bytesWritten++;
			}
			if (writeByte(PROT_TERM_CHAR)) {
				bytesWritten++;
			}
			return (bytesWritten == len + 1);
		}

		/** Simple processGetString_P that does not use a delegate. */
		bool processGetString_P(prot_cmd_t __cmdGet, PGM_P __str_P) {
			return test(BaseClass::reply(__cmdGet) && putString_P(__str_P));
		}

		/** Member function that processes get PROGMEM string requests. Must return
		a pointer to the string buffer.

		@param[out] __strbuf pointer to the string to get
		@return Must return true if successful
		*/
		struct GetStringFn_P {
			typedef bool (DEV::*type)(const char*& __strbuf);
		};

		/** Process a get array command. Calls a GetArrayFn. */
		bool processGetString_P(prot_cmd_t __cmdGet, typename GetStringFn_P::type __strbufFn_P) {
			if (!BaseClass::hasStarted()) {
				return false;
			}
			PGM_P strbuf_P;
			if (test(BaseClass::target_ && (BaseClass::target_ ->* __strbufFn_P)(strbuf_P))) {
				return test(BaseClass::reply(__cmdGet) && putString_P(strbuf_P));
			}
			return BaseClass::replyError();
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

#endif // #ifdef PROGMEM
	};

}; // namespace hprot

/////////////////////////////////////////////////////////////////////////////
/// Instantiate one definition rules. 
/// @see ArduinoCommon for explanation.
/////////////////////////////////////////////////////////////////////////////
#if defined(INSTANTIATE_STREAMHEXPROTOCOL_CPP) || defined(INSTANTIATE_ALL)

//### 
//### StreamHexProtocol is a template base class using CRTP. 
//### So nothing to INSTANTIATE
//### 

#endif // INSTANTIATE_STREAMHEXPROTOCOL_CPP


