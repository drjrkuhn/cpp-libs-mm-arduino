/**
\ingroup	ArduinoCommon
\file		DebugPrint.h
\brief		Can output extra debugging to a Print class
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: DebugPrint.h 430 2016-01-22 23:55:57Z jkuhn $
$Author: jkuhn $
$Revision: 430 $
$Date: 2016-01-22 17:55:57 -0600 (Fri, 22 Jan 2016) $

*/

#pragma once

#include <Stream.h>
#include <avr/pgmspace.h>
#include "../AsciiCodes.h"
#include <stdio.h> // for size_t
#include <HardwareSerial.h>


#define TEST_DEBUGPRINT_EXTRA	1

/** \ingroup ArduinoCommon

Read a character from program memory. 

The pgm_read_byte() macro is actually an assembly operation
which was screwing up code analysis. This is a "function"
version of that macro. 
*/
inline char read_P(PGM_P __p) {
	return pgm_read_byte(__p);
}

/** \ingroup ArduinoCommon

Extends a Stream class with more extensive debugging print capabilities.

Most public methods return a reference to `*this`, rather than the number
of characters printed (as most class Stream methods do). The returned reference
lets you re-use the DebugPrint object for the next operation, so 
operations can be chained together with a `.` operator. 

<br>
For example

\code{.cpp}
	DebugPrint out(Serial);
	out.print_P(PSTR("Hello, alternate-reality number ").print(7).print("!").println();
\endcode

<br>

printXXX methods 
-----------------

Send simple string tokens over the stream. 

The following flags functions control printXXX behavior:

\param enable() 	output is enabled **(default)**
\param disable() 	output is disabled (can be used to turn off logging)


logXXX methods 
------------------

Send formatted messages and error messages over the stream. 

Log behavior is controlled by setting the following flags, 
which can be set at creation or any time after.

\param logEndl() 			Add endline at the end of each logXXX
\param logSep()				logXXX ends in the specified separator string **(default with ", ")**
\param logStreams()			logXXX continuously output to the Stream **(default)**
\param flushLogAfter()	Each logXXX message calls flush() to clear the previous message
*/
class DebugPrint {
public:
	//////////////////////////////////////////////////////////
	/// \name Constructor
	///
	/// The default constructor takes an Stream reference.
	/// It defaults to no endlines at the end of a message and
	/// messages continuously print to the the stream without flushing.
	///
	/// Use one of the flag parameters to modify the default
	/// behavior
	///
	/// \code{.cpp}
	///		// Send debug messages to the Serial port
	///		DebugPrint out(Serial);
	///		out.logEndl();
	///
	///		// Send single debug messages to a StringStream
	///		StringStream lastMsg;
	///		DebugPrint out2(lastMsg);
	///		out2.flushLogAfter(1);
	///
	///		// Send several debug messages to a StringStream
	///		StringStream severalMsg;
	///		DebugPrint out3(lastMsg);
	///		out3.logSep(", ").flushLogAfter(5);
	/// \endcode
	///@{

	/** Constructor.
	@param __out Stream object to output to
	@param __disabled initially printing. */
	DebugPrint(Stream& __out)
		: out_(__out), disabled_(false), logCount_(0), logEndl_(false), flushLogEvery_(0), logSeparator_("; ") {}

	/** output is enabled **(default)** */
	DebugPrint& enable() {
		disabled_ = false;
		return *this;
	}

	/** output is disabled (can be used to turn off logging) */
	DebugPrint& disable() {
		disabled_ = true;
		return *this;
	}

	/** Add endline at the end of each logXXX */
	DebugPrint& logEndl() {
		logEndl_ = true;
		return *this;
	}

	/** logXXX ends in the specified separator string **(default with "; ")** */
	DebugPrint& logSep(String __sep) {
		logEndl_ = false;
		logSeparator_ = __sep;
		return *this;
	}

	/** logXXX continuously output to the Stream **(default)** */
	DebugPrint& logStreams() {
		flushLogEvery_ = 0;
		logCount_ = 0;
		return *this;
	}

	/** After this many logs, logXXX message calls flush() to clear the previous message.
	__cout = 0 disables flushing().
	*/
	DebugPrint& flushLogAfter(unsigned int __count) {
		flushLogEvery_ = static_cast<long>(__count);
		logCount_ = 0;
		return *this;
	}

#if TEST_DEBUGPRINT_EXTRA
	DebugPrint& markFlush(const char* __str) {
		flushMarker_ = __str;
		return *this;
	}
#endif

	Stream& getStream() const {
		return out_;
	}

	DebugPrint& flush() {
		out_.flush();
		logCount_ = 0;
		return *this;
	}

	///@}
	//////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////
	/// \name Simple printing
	///@{

	/** Add a newline at the end. */
	DebugPrint& endl() {
		if (disabled_) return *this;
		out_.println();
		return *this;
	}

	/** Interface to protected Stream::print functions taking a single argument */
	DebugPrint& print(bool __b) {
		if (disabled_) return *this;
		print_P(__b ? boolTrue_P : boolFalse_P);
		return *this;
	}


	/** Interface to protected Stream::print functions taking a single argument */
	template <typename T>
	DebugPrint& print(T __t) {
		if (disabled_) return *this;
		out_.print(__t);
		return *this;
	}

	/** Interface to protected Stream::print functions taking two arguments */
	template <typename T, typename U>
	DebugPrint& print(T __t, U __u) {
		if (disabled_) return *this;
		out_.print(__t, __u);
		return *this;
	}

	/** Interface to protected Stream::println functions taking a single argument */
	template <typename T>
	DebugPrint& println(T __t) {
		if (disabled_) return *this;
		out_.println(__t);
		return *this;
	}

	/** Interface to protected Stream::println functions taking two arguments */
	template <typename T, typename U>
	DebugPrint& println(T __t, U __u) {
		if (disabled_) return *this;
		out_.println(__t, __u);
		return *this;
	}

	/** Print the argument repeatedly */
	template <typename T>
	DebugPrint& printRepeat(T __t, int __count) {
		if (disabled_) return *this;
		while (__count--) {
			out_.print(__t);
		}
		return *this;
	}


	/** Print a string explicitly stored in PROGMEM. */
	// in INSTANTIATE
	DebugPrint& print_P(PGM_P __str_P);

	/** Print a string explicitly stored in PROGMEM repeatedly. */
	// in INSTANTIATE
	DebugPrint& printRepeat_P(PGM_P __str_P, int __count) {
		if (disabled_) return *this;
		while (__count--) {
			print_P(__str_P);
		}
		return *this;
	}

	/** Print a hex string with leading-zeros and the
	appropriate number of digits for the character class. */
	template <class T>
	DebugPrint& printHex(T __val) {
		if (disabled_) return *this;
		int digits = sizeof(T) * 2;
		while (digits--) {
			char hdig = (__val >> 4 * digits) & 0x0F;
			out_.write(static_cast<char>(hdig + (hdig > 9 ? 'A' - 10 : '0')));
		}
		return *this;
	}

	/** Print the contents of another Stream logger */
	DebugPrint& printStream(Stream& __stream) {
		if (disabled_) return *this;
		if (__stream.available()>0) {
			out_.print(__stream.readString());
		}
		return *this;
	}

	/** Print the contents of another DebugPrint logger */
	DebugPrint& printStream(DebugPrint& __other) {
		return printStream(__other.out_);
	}


	/** Print an ASCII character with control codes. 

	- `__fixedWidth = true` always prints two chars
		either a two char control code such as `^G`, a two-digit
		hex number for chars > 0x7F, or the char followed by a space.

	- `__fixedWidth = false` prints variable length
		either a bracket control code such as `[BEL]`, a hex
		number such as `[x9A]` for chars > 0x7F, or	the char with no space.

	@param __c character to print
	@param __fixedWidth flag that determines how the character or control code is printed.
	*/
	// in INSTANTIATE
	DebugPrint& printAscii(uint8_t __c, bool __fixedWidth = false);

	/** Print a fixed-width uint8_t buffer as decoded ASCII. */
	// in INSTANTIATE
	DebugPrint& printAscii(const uint8_t* __buf, size_t __size, bool __fixedWidth = false);

	/** Print a fixed-width char buffer as decoded ASCII. */
	DebugPrint& printAscii(const char* __str, size_t __size, bool __fixedWidth = false) {
		if (disabled_) return *this;
		return printAscii(reinterpret_cast<const uint8_t*>(__str), __size, __fixedWidth);
	}

	/** Print a char* string as decoded ASCII */
	DebugPrint& printAscii(const char* __str, bool __fixedWidth = false) {
		if (disabled_) return *this;
		return printAscii(__str, strlen(__str), __fixedWidth);
	}

	/** Print a string as decoded ASCII */
	DebugPrint& printAscii(const String __str, bool __fixedWidth = false) {
		if (disabled_) return *this;
		return printAscii(__str.c_str(), __str.length(), __fixedWidth);
	}

	/** Print a fixed-with char buffer stored in PROGMEM as decoded ASCII. */
	// in INSTANTIATE
	DebugPrint& printAscii_P(PGM_P __str_P, size_t __size, bool __fixedWidth = false);

	/** Print a string stored in PROGMEM as decoded ASCII. */
	DebugPrint& printAscii_P(PGM_P __str_P, bool __fixedWidth = false) {
		if (disabled_) return *this;
		return printAscii_P(__str_P, strlen_P(__str_P), __fixedWidth);
	}

	/** Print a string declared with the F() macro as decoded ASCII */
	DebugPrint& printAscii(const __FlashStringHelper* __str, bool __fixedWidth = false) {
		if (disabled_) return *this;
		return printAscii_P(reinterpret_cast<PGM_P>(__str), __fixedWidth);
	}

	///@}
	//////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////
	/// \name message logging
	///@{

	/** Used before starting a logXXX or custom log message. 
	Increments log count, Decides whether to flush out_, 
	and prints a separator if necessary. */
	DebugPrint& startLog();

	/** Used before starting a logErrorXXX or custom logError message.
	Prints the common "Error: " at the beginning of an error message. */
	DebugPrint& startLogError();

	/** Used after ending a logXXX message or custom log message. 
	Print a newline at the end of a logXXX if neccesarry. */
	DebugPrint& endLog();

	/** Print a log message */
	template<typename T>
	DebugPrint& logMsg(T __t) {
		if (disabled_) return *this;
		return startLog().print(__t).endLog();
	}

	/** Print a log message stored in PROGMEM. */
	DebugPrint& logMsg_P(PGM_P __str_P) {
		if (disabled_) return *this;
		return startLog().print_P(__str_P).endLog();
	}

	/** Print a log message with a second value */
	template<typename T, typename U>
	DebugPrint& logMsg(T __t, U __u) {
		if (disabled_) return *this;
		return startLog().print(__t).print_P(msgSep_P).print(__u).endLog();
	}

	/** Print a log message stored in PROGMEM with a second value. */
	template<typename U>
	DebugPrint& logMsg_P(PGM_P __str_P, U __u) {
		if (disabled_) return *this;
		return startLog().print_P(__str_P).print_P(msgSep_P).print(__u).endLog();
	}


	/** Print a log message with a buffer */
	template<typename T>
	DebugPrint& logMsgWithBuffer(T __t, const uint8_t* __buf, size_t __size, bool __fixedWidth=false) {
		if (disabled_) return *this;
		startLog().print(__t);
		return print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth).endLog();
	}

	/** Print a log message with a parameter and a buffer */
	template<typename T, typename U>
	DebugPrint& logMsgResultWithBuffer(T __t, U __u, const uint8_t* __buf, size_t __size, bool __fixedWidth=false) {
		if (disabled_) return *this;
		startLog().print(__t).print_P(msgSep_P).print(__u);
		return print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth).endLog();
	}

	/** Print a log message stored in PROGMEM with a buffer. */
	DebugPrint& logMsgWithBuffer_P(PGM_P __str_P, const uint8_t* __buf, size_t __size, bool __fixedWidth=false) {
		if (disabled_) return *this;
		startLog().print_P(__str_P);
		return print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth).endLog();
	}

	/** Print a log message stored in PROGMEM with a parameter and a buffer. */
	template<typename U>
	DebugPrint& logMsgResultWithBuffer_P(PGM_P __str_P, U __u, const uint8_t* __buf, size_t __size, bool __fixedWidth=false) {
		if (disabled_) return *this;
		startLog().print_P(__str_P).print_P(msgSep_P).print(__u);
		return print_P(msgSep_P).printAscii(__buf, __size, __fixedWidth).endLog();
	}

	/** Print an Error: log message */
	template<typename T>
	DebugPrint& logErrorMsg(T __t) {
		if (disabled_) return *this;
		return startLogError().print(__t).endLog();
	}

	/** Print an Error: log message stored in PROGMEM. */
	DebugPrint& logErrorMsg_P(PGM_P __msg_P) {
		if (disabled_) return *this;
		return startLogError().print_P(__msg_P).endLog();
	}

	/** Print an Error: log message and second value*/
	template<typename T, typename U>
	DebugPrint& logErrorMsg(T __t, U __u) {
		if (disabled_) return *this;
		return startLogError().print(__t).print_P(msgSep_P).print(__u).endLog();
	}

	/** Print an Error: log message stored in PROGMEM and a second value. */
	template<typename U>
	DebugPrint& logErrorMsg_P(PGM_P __msg_P, U __u) {
		if (disabled_) return *this;
		return startLogError().print_P(__msg_P).print_P(msgSep_P).print(__u).endLog();
	}

	/** Print an Error: log message with the second value as a HEX */
	template<typename T, typename U>
	DebugPrint& logErrorMsgHex(T __t, U __u) {
		if (disabled_) return *this;
		return startLogError().print(__t).print_P(msgSep_P).print('x').printHex(__u).endLog();
	}

	/** Print an Error: log message stored in PROGMEM and a second value as a HEX. */
	template<typename U>
	DebugPrint& logErrorMsgHex_P(PGM_P __msg_P, U __u) {
		if (disabled_) return *this;
		return startLogError().print_P(__msg_P).print_P(msgSep_P).print('x').printHex(__u).endLog();
	}

	///@}
	//////////////////////////////////////////////////////////


protected:
	//////////////////////////////////////////////////////////
	// Protected members
	//////////////////////////////////////////////////////////

	/** Prints the size followed by `:\"`. Common start for printAscii strings */
	bool printAsciiStart(size_t __size);

	Stream& out_;

	unsigned long logCount_;	///< number of logXXX messages since last flush()

	/** Flag for printing and logging */
	bool disabled_;		///< output is disabled
	bool logEndl_;		///< add end-line after any logXXX method. Useful when out = Serial
	unsigned long flushLogEvery_;	///< Flush after log count reaches this number. 0 to disable
	String logSeparator_;			///< Separator string between log messages when not logEndl_

	static const char emptyBuffer_P[] PROGMEM;
	static const char controlCodes_P[] PROGMEM;
	static const char deleteCode_P[] PROGMEM;
	static const char error_P[] PROGMEM;
	static const char msgSep_P[] PROGMEM;
	static const char boolTrue_P[] PROGMEM;
	static const char boolFalse_P[] PROGMEM;
#if TEST_DEBUGPRINT_EXTRA
	const char* flushMarker_ = nullptr;
#endif
};

/////////////////////////////////////////////////////////////////////////////
/// Instantiate one definition rules. 
/// @see ArduinoCommon for explanation.
/////////////////////////////////////////////////////////////////////////////
#if defined(INSTANTIATE_DEBUGPRINT_CPP) || defined(INSTANTIATE_ALL) || defined(__INTELLISENSE__)

/** Special string printed for printAscii when the buffer size is zero. */
const char DebugPrint::emptyBuffer_P[] PROGMEM = "0:[empty]";

/**
ASCII control codes are stored in one big string with 5 characters per code.
The first 3 characters are a "human readable" format, while
the next two characters are the standard "control" codes.
*/
const char DebugPrint::controlCodes_P[] PROGMEM =
/*00*/ "nul^@" /*01*/ "SOH^A" /*02*/ "STX^B" /*03*/ "ETX^C"
/*04*/ "EOT^D" /*05*/ "ENQ^E" /*06*/ "ACK^F" /*07*/ "BEL^G"
/*08*/ "BS ^H" /*09*/ "TAB^I" /*0a*/ "NL ^J" /*0b*/ "VT ^K"
/*0c*/ "FF ^L" /*0d*/ "CR ^M" /*0e*/ "SO ^N" /*0f*/ "SI ^O"
/*10*/ "DLE^P" /*11*/ "DC1^Q" /*12*/ "DC2^R" /*13*/ "DC3^S"
/*14*/ "DC4^T" /*15*/ "NAK^U" /*16*/ "SYN^V" /*17*/ "ETB^W"
/*18*/ "CAN^X" /*19*/ "EM ^Y" /*1a*/ "SUB^Z" /*1b*/ "ESC^["
/*1c*/ "FS ^\\" /*1d*/ "GS ^]" /*1e*/ "RS ^^" /*1f*/ "US ^_";

const char DebugPrint::deleteCode_P[] PROGMEM = /*7f*/ "DEL^#";

const char DebugPrint::error_P[] PROGMEM = "Error";
const char DebugPrint::msgSep_P[] PROGMEM = ": ";
const char DebugPrint::boolTrue_P[] PROGMEM = "true";
const char DebugPrint::boolFalse_P[] PROGMEM = "false";


DebugPrint& DebugPrint::print_P(PGM_P __str_P) {
	if (disabled_) return *this;
	char c;
	while ((c = read_P(__str_P++))) {
		out_.write(c);
	}
	return *this;
}

DebugPrint& DebugPrint::printAscii(uint8_t __c, bool __fixedWidth) {
	if (disabled_) return *this;
	char cc;
	if (__c < ASCII_MIN_TEXT || __c == ASCII_DEL) {
		int i = 0, cs = 5 * __c;
		PGM_P codes_P = controlCodes_P;
		if (__c == ASCII_DEL) {
			cs = 0;
			codes_P = deleteCode_P;
		}
		if (__fixedWidth) {
			for (cs += 3; i < 2; i++) {
				out_.write(static_cast<char>(read_P(codes_P + cs + i)));
			}
		} else {
			out_.write('[');
			for (; i < 3; i++) {
				cc = read_P(codes_P + cs + i);
				if (cc != ' ') {
					out_.write(cc);
				}
			}
			out_.write(']');
		}
	} else if (__c <= ASCII_MAX_TEXT) {
		out_.write(static_cast<char>(__c));
		if (__fixedWidth) {
			out_.write(' ');
		}
	} else {
		if (!__fixedWidth) {
			out_.write('[');
			out_.write('x');
		}
		printHex<uint8_t>(__c);
		if (!__fixedWidth) {
			out_.write(']');
		}
	}
	return *this;
}


DebugPrint& DebugPrint::printAscii(const uint8_t* __buf, size_t __size, bool __fixedWidth) {
	if (disabled_) return *this;
	if (printAsciiStart(__size)) {
		while (__size--) {
			printAscii(*(__buf++), __fixedWidth);
		}
	}
	out_.write('\"');
	return *this;
}

DebugPrint& DebugPrint::printAscii_P(PGM_P __str_P, size_t __size, bool __fixedWidth) {
	if (disabled_) return *this;
	if (printAsciiStart(__size)) {
		while (__size--) {
			printAscii(read_P(__str_P++), __fixedWidth);
		}
	}
	out_.write('\"');
	return *this;
}

bool DebugPrint::printAsciiStart(size_t __size) {
	if (disabled_) return false;
	if (__size == 0) {
		print_P(emptyBuffer_P);
		return false;
	}
	out_.print(__size);
	out_.write(':');
	out_.write('\"');
	return true;
}

/** Print Decides whether to flush out_ */
DebugPrint& DebugPrint::startLog() {
	if (disabled_) return *this;
	logCount_++;
	if (flushLogEvery_ > 0 && logCount_ > flushLogEvery_) {
		out_.flush();
#if TEST_DEBUGPRINT_EXTRA
		if (flushMarker_ && flushMarker_[0] != '\0') {
			print(flushMarker_);
		}
#endif
		logCount_ = 1;
	}
	if (!logEndl_ && logCount_ > 1) {
		//print('(').print(logCount_).print(')');
		print(logSeparator_);
	}
	//print(" >>");
	return *this;
}

DebugPrint& DebugPrint::startLogError() {
	if (disabled_) return *this;
	return startLog().print_P(error_P).print_P(msgSep_P);
}

/** print a newline or separator at the end of a logXXX . */
DebugPrint& DebugPrint::endLog() {
	if (disabled_) return *this;
	//print("<< ");
	if (logEndl_) {
		endl();
	}
	return *this;
}

#endif // INSTANTIATE_DEBUGPRINT_CPP

//////////////////////////////////////////////////////////
/// Test Code
//////////////////////////////////////////////////////////
#if defined(TEST_DEBUGPRINT) || defined(__INTELLISENSE__)
#include <Arduino.h>
#include <HardwareSerial.h>

#define BUFFER_WITH_CODES "\x02" "Ascii buffer \tcontaining " "\x07" "\r\n several\0" " unprintable\x7f" " control" "\x7c"" codes.\xAB\xCD\xEF\x04"
const char testBuffer[] = BUFFER_WITH_CODES;
const size_t testLen = sizeof(BUFFER_WITH_CODES)-1;
const char testBuffer_P[] PROGMEM = BUFFER_WITH_CODES;

#define INT_MSG		12345
#define CHAR_MSG	"const char* message"
#define STRING_MSG	String("String message")
#define F_MSG		F("F message")
#define PSTR_MSG	PSTR("PSTR message")


const char testStr1_P[] PROGMEM = "Flash 1";

void printTest(DebugPrint& __out) {
	__out.endl().print(F("Test print")).endl();
	__out.printRepeat("=", 40).endl();
	__out.print(F("100 = ")).print(100, DEC).endl();
	__out.print(F("Flash 1 = ")).print_P(testStr1_P).endl();
	__out.print(F("Flash 2 = ")).print_P(PSTR("Flash 2")).endl();
	__out.print(F("Flash 3 = ")).print(F("Flash 3")).endl();
	__out.endl();
}

void printHexTest(DebugPrint& __out) {
	__out.print(F("Test printHex")).endl();
	__out.printRepeat("=", 40).endl();
	__out.print(F("01 = ")).printHex(uint8_t(0x1)).endl();
	__out.print(F("12 = ")).printHex(uint8_t(0x12)).endl();
	__out.print(F("00AB = ")).printHex(uint16_t(0xab)).endl();
	__out.print(F("0123 = ")).printHex(uint16_t(0x123)).endl();
	__out.print(F("1234 = ")).printHex(uint16_t(0x1234)).endl();
	__out.print(F("01234567 = ")).printHex(uint32_t(0x01234567)).endl();
	__out.print(F("89ABCDEF = ")).printHex(uint32_t(0x89abcdef)).endl();
	__out.print(F("123456789ABCDEF0 = ")).printHex(uint64_t(0x123456789abcdef0)).endl();
	__out.endl();
}

void printAsciiTest(DebugPrint& __out, bool __fixedWidth) {
	__out.print(F("Test printAscii, fixedWidth=")).print(__fixedWidth).endl();
	__out.printRepeat("=", 40).endl();

	__out.print('\"').print(testBuffer).print('\"').endl();
	__out.printAscii(reinterpret_cast<const uint8_t*>(testBuffer), testLen, __fixedWidth).endl();
	__out.printAscii(testBuffer, testLen, __fixedWidth).endl();
	__out.printAscii(testBuffer, __fixedWidth).endl();
	__out.printAscii(String(BUFFER_WITH_CODES), __fixedWidth).endl();
	__out.printAscii_P(testBuffer_P, testLen, __fixedWidth).endl();
	__out.printAscii_P(testBuffer_P,  __fixedWidth).endl();
	__out.printAscii_P(PSTR(BUFFER_WITH_CODES), __fixedWidth).endl();
	__out.printAscii(F(BUFFER_WITH_CODES), __fixedWidth).endl();
	__out.endl();
}

void logMsgTest(DebugPrint& __out, bool __logEndl, String __sep, long __flushAfter) {

	__out.endl().endl().print(F("Test logMsg endl=")).print(__logEndl);
	__out.print(" sep=\"").print(__sep).print("\", flushAfter=").print(__flushAfter).endl();
	__out.printRepeat("=", 40).endl();

	if (__logEndl) {
		__out.logEndl();
	} else {
		__out.logSep(__sep);
	}
	if (__flushAfter > 0) {
		__out.flushLogAfter(__flushAfter);
	} else {
		__out.logStreams();
	}
#if TEST_DEBUGPRINT_EXTRA
	__out.markFlush("[FLUSH]\n");
#endif

	__out.logMsg(INT_MSG);
	__out.logMsg(CHAR_MSG);
	__out.logMsg(STRING_MSG);
	__out.logMsg(F_MSG);
	__out.logMsg_P(PSTR_MSG);
	if (__logEndl) __out.endl();

	__out.logErrorMsg(INT_MSG);
	__out.logErrorMsg(CHAR_MSG);
	__out.logErrorMsg(STRING_MSG);
	__out.logErrorMsg(F_MSG);
	__out.logErrorMsg_P(PSTR_MSG);
	if (__logEndl) __out.endl();

	__out.logErrorMsg(INT_MSG, 3.14);
	__out.logErrorMsg(CHAR_MSG, 3.14);
	__out.logErrorMsg(STRING_MSG, 3.14);
	__out.logErrorMsg(F_MSG, 3.14);
	__out.logErrorMsg_P(PSTR_MSG, 3.14);
	if (__logEndl) __out.endl();

	__out.logErrorMsgHex(INT_MSG, 0xabcd);
	__out.logErrorMsgHex(CHAR_MSG, 0xabcd);
	__out.logErrorMsgHex(STRING_MSG, 0xabcd);
	__out.logErrorMsgHex(F_MSG, 0xabcd);
	__out.logErrorMsgHex_P(PSTR_MSG, 0xabcd);
	if (__logEndl) __out.endl();

#if TEST_DEBUGPRINT_EXTRA
	__out.markFlush("");
#endif
	__out.endl();
}

void allTests(DebugPrint& __out, bool __disableOut) {
	__out.endl().print(F("Starting All Tests, disabled=")).print(__disableOut).endl();
	__out.printRepeat("#", 50).endl();
	if (__disableOut) {
		__out.disable();
	}
	printTest(__out);
	printHexTest(__out);
	printAsciiTest(__out, false);
	printAsciiTest(__out, true);
	logMsgTest(__out, true, "", 0);
	logMsgTest(__out, false, " @ ", 0);
	logMsgTest(__out, false, " @ ", 3);
	logMsgTest(__out, false, " @ ", 1);
	__out.enable();
	__out.endl().printRepeat("#", 50).endl();
	__out.print(F("All Tests Done, disabled=")).print(__disableOut).endl();
}

extern "C" {
	void setup() {
		Serial.begin(115200);
		delay(200);

		DebugPrint out(Serial);
		allTests(out, false);
		allTests(out, true);

		out.endl().printRepeat("@", 80).endl();
		out.print(F("StringStream logging")).endl();
		out.printRepeat("@", 80).endl();
		StringStream ios(1000);
		DebugPrint osout(ios);

		osout.print_P(PSTR("Hello world"));

		out.print(ios.str()).endl();
		osout.flush();

		printTest(osout);
		out.print(ios.str()).endl();

		printHexTest(osout);
		out.print(ios.str()).endl();
		printAsciiTest(osout, false);
		out.print(ios.str()).endl();
		printAsciiTest(osout, true);
		out.print(ios.str()).endl();
	}

	void loop() {
	}

}; // extern "C"
#endif // TEST_DEBUGPRINT


