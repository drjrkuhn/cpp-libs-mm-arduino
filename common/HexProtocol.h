/**
\defgroup	HexProtocol		Hexadecimal Protocol

Defines a serial communication protocol between host and slave
*/

/**
\ingroup	HexProtocol
\file		HexProtocol.h
\brief		Defines a serial communication protocol between host and slave
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: HexProtocol.h 695 2017-05-20 18:51:47Z jkuhn $
$Author: jkuhn $
$Revision: 695 $
$Date: 2017-05-20 13:51:47 -0500 (Sat, 20 May 2017) $

*/

/**
\ingroup HexProtocol

\page AboutHexProtocol About the Hexadecimal Protocol

Hexadecimal Serial Communication Protocol between a Host and a Slave device.

Protocol Basics
=============================================================================

I appear to have settled on a good, speedy protocol for sending commands
to the arduino and getting responses. The methods defined in HexProtocol
below are [mostly] debugged and take care of the nitty-gritty details of 
passing information over the serial line. 

The basic protocol HexProtocol contract
----------------------------------------------------------

- Commands are always sent as singe, non-terminated bytes.

- Single-byte commands sent from the host to the slave are the ONLY non-terminated
  transmissions. The slave may simple check for a new byte in its receive
  buffer to start the response/update process.

- Commands can be followed by HEX-encoded and [EOT] terminate strings. 

- The TERM char is nominally the ASCII [EOT] character (0x04) as \c \#defined below

- Values and responses are always sent as HEX-encoded, [EOT] terminated integers 
  or float, or [EOT] terminated strings.

- Typical SET_VALUE and GET_VALUE command sequences are shown below

- Channel commands are immediately followed by a HEX-encoded channel for which they will apply


Protocl SET, GET, and TASK examples
=============================================================================

First, a few notes on nomenclature used in the examples below

- Commands protocols are given in "lambda" notation
	+ CMD(arg) -> (ret)	is a command that takes one argument and returns one value
	+ CMD() -> (ret)	is a command that takes no arguments and returns one value
	+ CMD(arg) -> ()	is a command that takes one argument and returns nothing
	+ CMD() -> ()		is a command that takes no arguments and returns nothing

- byte:xx indicates a single byte value (command)

- HEX:xxx[EOT] indicates an integer or float encoded as a HEX string
  such as `12FDE34C` terminated with an [EOT] character. The HEX string
  may be preceded by a negative sign '-' if the integer TYPE was signed
  and the value was negative. 

 - Values may also be encoded as an [EOT]-terminated string instead of a 
   HEX encoded integer or float.

A SET_VALUE(value)->() command from the host looks like this:
-----------------------------------------------------------------------------

1. HOST sends single byte:'SET_CMD', where SET_CMD is value-specific

2. HOST encodes value as HEX and sends HEX:value[EOT] with putValue<T>() 

3. HOST waits for an [EOT] terminated response with checkReply(SET_CMD)

4. SLAVE hasCommand() changes to true

5. SLAVE receives byte:'SET_CMD' with getCommand()

6. SLAVE attempts to receive value with getValue<T>() and does something with it

7. SLAVE sends

	+ if all was OK:
	  SLAVE sends HEX:SET_CMD[EOT] back to the host with reply(SET_CMD) [which calls putValue<T>(SET_CMD)]

	+ if an error occured:
	  SLAVE sends HEX:PROT_ERROR[EOT] back to the host with replyError(), which calls reply(PROT_ERROR)

8. HOST receives response via checkReply() and checks if it was the SET_CMD or a PROT_ERROR

This can be shortened with distpatchSet and processSet using the following pseudocode

\code
	HOST calls dispatchSet(), which calls
		//NOTE: the next function in the chain will only be called if the previous was true
		putCommand(SET_CMD) && putValue(value) && checkReply(SET_CMD)
	SLAVE calls getCommand(), which eventually calls processSet(), which does
		if (getValue(value) && doSetFunction(value)) {
			reply(SET_CMD);
		} else {
			replyError();
		}
\endcode

A GET_VALUE()->(value) command from the host looks like this:
-----------------------------------------------------------------------------

1. HOST sends single byte:'GET_CMD', where GET_CMD is value-specific

2. HOST waits for an [EOT] terminated response with checkReply()

3. SLAVE hasCommand changes to true

4. SLAVE receives byte:'GET_CMD' with getCommand()

5. SLAVE sends

	+ if all was OK:
	  SLAVE sends HEX:GET_CMD[EOT] back to the host with reply(GET_CMD) [which calls putValue<T>(GET_CMD)]
	  SLAVE encodes value as HEX and sends HEX:value[EOT] with putValue<T>()

	+ if an error occurred:
	  SLAVE sends HEX:PROT_ERROR[EOT] back to the host with replyError()

6. HOST receives response via checkReply(GET_CMD)

	+ if response was GET_CMD
	  HOST receives value with getValue<T>() and does something with it

	+ if an error occurred:
	  HOST does not wait for a value

This can be shortened with distpatchGet and processGet

\code
	HOST calls dispatchGet(), which calls
		//NOTE: the next function in the chain will only be called if the previous was true
		putCommand(GET_CMD) && checkReply(GET_CMD) && getValue(value)
	SLAVE calls getCommand(), which eventually calls processGet(), which does
		if (doGetFunction(value)) {
			//NOTE: the next function in the chain will only be called if the previous was true
			reply(GET_CMD) && putValue(value)
		} else {
			replyError();
		}
\endcode

A TASK()->() command from the host without values looks like this:
-----------------------------------------------------------------------------

1. HOST sends single byte:'TASK_CMD', where TASK_CMD is specific to the task

2. HOST waits for an [EOT] terminated response with checkReply(TASK_CMD)

3. SLAVE hasCommand() changes to true

4. SLAVE receives byte:'TASK_CMD' with getCommand()

5. SLAVE sends

	+ if all was OK:
	  SLAVE sends HEX:TASK_CMD[EOT] back to the host with reply(TASK_CMD) [which calls putValue<T>()]

	+ if an error occured:
	  SLAVE sends HEX:PROT_ERROR[EOT] back to the host with replyError()

6. HOST receives response via checkReply() and checks if it was the TASK_CMD or a PROT_ERROR

This can be shortened with distpatchTask and processTask

\code
	HOST calls dispatchTask(), which calls
		//NOTE: the next function in the chain will only be called if the previous was true
		putCommand(TASK_CMD) && checkReply(TASK_CMD)
	SLAVE calls getCommand(), which eventually calls processTask(), which does
		if (doTask()) {
			reply(TASK_CMD);
		} else {
			replyError();
		}
\endcode

SUB-COMMANDS
=============================================================================

Some commands also have sub-commands. These are commands followed by a standard 
HEX-encoded and null terminated sub-command value. The SLAVE device performs the
SET or GET operation for the subcommand, and replies with the original SET or GET
command (not the sub-command) or a PROT_ERROR.
Sub-commands are primarily used for passing arrays between the host and slave.

A SET : SUB_VALUE(value)->() command from the host looks like this:
-----------------------------------------------------------------------------

\code
	HOST calls dispatchSet(), which calls
		//NOTE: the next function in the chain will only be called if the previous was true
		putCommand(SETSUB_CMD) && putValue(SUB_CMD) && putValue(value) && checkReply(SETSUB_CMD)
	SLAVE calls getCommand(), which eventually leads to processSubCmd, which does
		if (getValue(subCmd)) {
			if (subCmd == SUB_CMD) {
				if (getValue(value) && doSubSetFunction(value)) {
					reply(SETSUB_CMD);
				} else {
					replyError();
				}
			}
			...check other SUB_CMD
\endcode
A GET:SUB_VALUE()->(value) command from the host looks like this:
-----------------------------------------------------------------------------

\code
	HOST calls dispatchSet(), which calls
		//NOTE: the next function in the chain will only be called if the previous was true
		putCommand(GETSUB_CMD) && putValue(SUB_CMD) && checkReply(GETSUB_CMD) && getValue(value) 
	SLAVE calls getCommand(), which eventually leads to processSubCmd, which does
		if (getValue(subCmd) {
			if (subCmd == SUB_CMD) {
				if (doSubGetFunction(value)) {
					//NOTE: the next function in the chain will only be called if the previous was true
					reply(GETSUB_CMD) && putValue(value)
				} else {
					replyError();
				}
			}
			...check other SUB_CMD
\endcode

Passing Values through the protocol
=============================================================================

Integer and Float type bits
-----------------------------------------------------------------------------

The AVR libc does not contain strtoull or strtoll (64-bit integers). In fact,
64-bit numbers are not always available on the AVR side.
So, transmission is limited to 32-bits (strtol). This means we can transmit
and receive float's (32-bits) as IEEE-754 hex numbers, but NOT double's (64-bits).
With that said, double's on the AVR are generally 32-bit, NOT 64-bit. So on the Arduino,
sizeof(double) == sizeof(float). So, we should stick with floats for transmitting.

NOTES on IEEE-754 floating point values
-----------------------------------------------------------------------------

If PROT_FLOAT_IEEE754 is defined, floats will be sent as 32-bit binary
values (8 hex digits). The AVR stores floats and doubles as single-precision
32-bit IEEE-754. The host must support single-precision IEEE-754 format
as well.

If PROT_FLOAT_IEEE754 is undefined, floats will be sent as long text
strings using the "exponential" `-1.2345678e+090` representation.
Tests of the AVR show that a precision of 7 characters after the decimal
are sufficient to represent 32-bit floats (23 bits of mantissa). So, for
a full floating point representation with 7 places of precision, we need
a buffer to hold -X.1234567e+XXX or 3 + 7 + 5 = 15 characters

Calling member functions through dispatchXXX methods
==============================================

Protocol commands must sometimes be given class member functions as
arguments. If the Arduino supported C++11, we could use lamba functions
with a capture list of [this] to pass member functions. Since
we do not have that luxury. Instead, we will use templates to
call a member function pointer through a target (this) pointer.
The declaration style for member function pointers is a little
counter-intuitive.

@see [C++ Tutorial: Pointer-to-Member Function]
(http://www.codeguru.com/cpp/cpp/article.php/c17401/C-Tutorial-PointertoMember-Function.htm)

Given a non-member function (just a straight function in glocal address space), you can create
a pointer to that function and use it with

\code{.cpp}
	int fooFunction(float value) {
		return static_cast<int>(value*3.14);
	}

	// create a pointer to the function. Note that 'fooptr' is the name of the variable
	// we create, and it goes in the middle of the function pointer variable declaration.
	int (*fooptr)(float v) = &fooFunction;

	// call it by dereferencing 'fooptr'
	int retval = (*fooptr)(2.0);
\endcode

For a **class member function**, the situation is a little different. The function pointer
has a different signature **and** you an instance of the to call it on. You can either use
a class variable or a pointer to a class variable. Each uses a slightly different notation
to call the function, (a.*fn)(args), vs (ptr_a->*fn)(args). I will give both examples below.

\code{.cpp}
	class FooClass {
	public:
	int fooMemberFunction(float value) {
		return static_cast<int>(value*multiplier_);
	}
	double multiplier_ = 2.718;
	};

	// create a pointer to the member function
	int (FooClass::*fooptr)(float v) = &FooClass::fooMemberFunction;

	// create a class variable and call the member function
	A	a();
	int retval = (a.*fooptr)(2.0);

	// create a pointer to a class variable and call the member function
	A*	ptr_to_a = &a;
	retval = (ptr_to_a->*fooptr)(2.0);
\endcode

Conditional Chaining and short-circuit evaluation
====================================================

Most of the functions in HexProtocol return a boolean true if the function 
was successful, or false otherwise. This lets us take advantage of C++'s 
short-circuit evaluation standard to have each send/receive step
depend on the success of the previous step. See [Short-circuit evaluation]
(https://en.wikipedia.org/wiki/Short-circuit_evaluation)

In short, boolean && evaluation sequences go from left to right
and the evaluation stops at the first false. For example, if the
following boolean functions are chained with &&,

\code{.cpp}
	tryFirst() && trySecond() && tryThird() && tryFourth()
\endcode

if trySecond() returns false, then tryThird() and tryFourth() are
never executed!
We can determine if a chained sequence was successful with

\code{.cpp}
	if ( tryFirst() && trySecond() && tryThird() && tryFourth() ) {
		// Success!!!
	} else {
		// Failure :-(
	}
\endcode

Conversely we can determine if a sequence fails with the construct

\code{.cpp}
	if (!( tryFirst() && trySecond() && tryThird() && tryFourth() )) {
		// Failure :-(
	} else {
		// Success!!!
	}
\endcode

Note that || chaining stops at the first true, so we could have written
the the above as the negated version

\code{.cpp}
	if (!tryFirst() || !trySecond() || !tryThird() || !tryFourth() )) {
		// Failure :-(
	}
\endcode

I prefer the && method for clarity of intent. However, the `if(!(` syntax
can lead to problems if the second parenthesis is missing. For example
\code{.cpp}
    // Bad short-circuit evaluation
    if (!tryFirst() && trySecond() ) {
        // Failure clause likely never reached
        // 
	} else {
		// Not true success
	}
\endcode
 
Note the mistake of `if(!` instead of `if(!(`. The failure clause
is only reached if tryFirst() failed and trySecond() succeeded,
a *very* unlikely scenario. The test function below can be
used as syntactic sugar to enclose a command chain and
specify intent. It is much easier to find coding mistakes
if every command chain is enclosed in a test clause.

\code{.cpp} 
    if (!test(doFirst() && doSecond()) {
        // failure clause
    } else {
        // success clause
    }
\endcode

*/

#pragma once

#include "AsciiCodes.h"

//////////////////////////////////////////////////////////////////////////
/// \name common access to standard integer types, uint8_t, etc.
/// \ingroup	HexProtocol 
///@{

#ifdef __AVR__
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h> // for size_t
/** The avr (Arduino) libray defines standard integer types uint8_t, etc
without namespace std, so we define std as blank here to use the global
address space ::uint8_t, etc on the avr side */
#define std						
#else // NOT #ifdef __AVR__



#include <cstdint>
#endif // #ifdef __AVR__



///@}

namespace hprot {

	//////////////////////////////////////////////////////////////////////////
	/// \name Defined sizes of the various pre-defined values used in the protocol
	/// \ingroup	HexProtocol 
	///@{

	/** A single byte */
	typedef std::uint8_t prot_byte_t;
	/** A boolean value */
	typedef std::uint8_t prot_bool_t;
	/** Commands are always single bytes */
	typedef std::uint8_t prot_cmd_t;
	/** Channels are always single bytes */
	typedef std::int8_t prot_chan_t;
	/** The maximum type of signed integer */
	typedef std::int32_t prot_long_t;
	/** The maximum type of unsigned integer */
	typedef std::uint32_t prot_ulong_t;
	/** Standard floating-point type */
	typedef float prot_float_t;
	/** standard size_t type *only* used for passing buffer sizes back and forth. Internal
	methods will use whichever native size_t is defined */
	typedef std::uint16_t prot_size_t;

#ifdef __AVR__
	/** protocol strings are usually StringObject on the arduino side. */
	typedef String prot_string_t;
#else // NOT #ifdef __AVR__



	/** protocol strings are std::string on the host side */
	typedef std::string prot_string_t;
#endif // #ifdef __AVR__



	///@}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/// \name macro definitions and constants
	/// \ingroup	HexProtocol 
	///@{

#define PROT_FLOAT_IEEE754									///< \c \#undef to pass floats as numeric text strings

#define PROT_ERROR				ASCII_NAK					///< protocol error command
#define PROT_TERM_CHAR			ASCII_EOT					///< all transmissions end in an ASCII EOT character
#define PROT_RADIX				16							///< transmit HEX characters
#define IS_SIGNED(TYPE)			((TYPE)(-1)<(TYPE)(0))		///< Helper macro to test if a type supports signed values

	/** Maximum integer hex digits in protocol. Add 2 bytes for possible negative sign and null term. */
	const size_t PROT_HEX_BUFF_SIZE = (2 * sizeof(prot_ulong_t) + 2);

	///@}
	//////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	/// \name Floating point buffer size
	/// \ingroup	HexProtocol 
	///@{

#ifdef PROT_FLOAT_IEEE754

	/** Maximum prot_float_t hex digits in protocol. Same as uint32_t. */
	const size_t PROT_FLOAT_BUFF_SIZE = PROT_HEX_BUFF_SIZE;

#else // NOT #ifdef PROT_FLOAT_IEEE754



	/** Default precision for textual floating point numbers. Only used by the protocol when PROT_FLOAT_IEEE754 is not defined. */
	const int PROT_FLOAT_MAX_PREC = 7;

	/**A little extra length for safety. Only used by the protocol when PROT_FLOAT_IEEE754 is not defined. */
	const int PROT_DEC_FLOAT_EXTRA = 4;

	/** Maximum prot_float_t hex digits in protocol. Maximum length for textual floating point numbers. Only used by the protocol when PROT_FLOAT_IEEE754 is not defined. */
	const size_t PROT_FLOAT_BUFF_SIZE = 3 + PROT_FLOAT_MAX_PREC + 5 + PROT_DEC_FLOAT_EXTRA;

#endif // #ifdef PROT_FLOAT_IEEE754



	///@}	
	///////////////////////////////////////////////////////////////////


#ifdef __AVR__
	//////////////////////////////////////////////////////////////
	/// \name long, unsigned long, and float to/from strings
	/// \ingroup	HexProtocol 
	///@{
#include <stdint.h>

	/** Helper to convert longs to text using PROT_RADIX. The __size must be >= PROT_HEX_BUFF_SIZE. Uses avr libc version of ltoa. */
	inline char* prot_ltohexstr(long __value, char* __buf, size_t __size) {
		if (__size < PROT_HEX_BUFF_SIZE) {
			__buf[0] = '\0';
			return __buf;
		}
		return ltoa(__value, __buf, PROT_RADIX);
	}

	/** Helper to convert unsigned longs to text using PROT_RADIX. The __size must be >= PROT_HEX_BUFF_SIZE. Uses avr libc version of ultoa */
	inline char* prot_ultohexstr(unsigned long __value, char* __buf, size_t __size) {
		if (__size < PROT_HEX_BUFF_SIZE) {
			__buf[0] = '\0';
			return __buf;
		}
		return ultoa(__value, __buf, PROT_RADIX);
	}

#ifndef PROT_FLOAT_IEEE754
	/** helper to convert floats to text (1.234e56). Uses the avr libc dtostre function. Must include libm.a in firmware project. */
inline char* prot_ftostr(float __val, char* __buf, size_t __size, unsigned char __prec) {
	if (__size < PROT_FLOAT_BUFF_SIZE) {
		__buf[0] = '\0';
		return __buf;
	}
	if (__prec > PROT_FLOAT_MAX_PREC) {
		__prec = PROT_FLOAT_MAX_PREC ;
	}
	return dtostre(__val, __buf, __prec, 0);
}

	/** Helper to convert text to float (1.234e56). Uses avr libc version of atof. */
inline double prot_strtof(const char* __str) {
	return atof(__str);
}

#endif // #ifdef PROT_FLOAT_IEEE754



	///@}

#else // NOT #ifdef __AVR__



	//////////////////////////////////////////////////////////////
	/// \name long, unsigned long, and float to/from strings
	/// \ingroup	HexProtocol 
	/// Host (PC) version specifics
	///@{
	/** Helper to convert longs to text. Uses cstdlib version of ltoa. The __size must be >= PROT_HEX_BUFF_SIZE. */
	inline char* prot_ltohexstr(long __val, char* __buf, size_t __size) {
		if (__size < PROT_HEX_BUFF_SIZE) {
			__buf[0] = '\0';
			return __buf;
		}
		return _ltoa(__val, __buf, PROT_RADIX);
	}

	/** helper to convert unsigned longs to text. Uses stdlib version of ultoa. The __size must be >= PROT_HEX_BUFF_SIZE. */
	inline char* prot_ultohexstr(unsigned long __val, char* __str, int __base) {
		return _ultoa(__val, __str, __base);
	}

#ifndef PROT_FLOAT_IEEE754
	/** use cstdlib version atof */
inline double prot_strtof(const char* __str) {
	return atof(__str);
}

	/** helper to convert floats to text (1.234e56) using sprintf. */
inline char* prot_ftostr(float __val, char* __buf, size_t __size, unsigned char __prec) {
	if (__prec > PROT_FLOAT_MAX_PREC) {
		__prec = PROT_FLOAT_MAX_PREC;
	}
	// use sprintf to mimic the dtostre function of the AVR library
	sprintf_s(__buf, __size, "%.*e", __prec, __val);
	return __buf;
}
#endif // #ifdef PROT_FLOAT_IEEE754



	///@}
	//////////////////////////////////////////////////////////////

#endif // #ifdef __AVR__



	/**
	\ingroup HexProtocol
	
	\page AboutSubCommands	About Sub-command

	About Sub-commands

	Sub-commands for array operations
	=======================================

	We don't want to stream an entire array at once over the serial line.
	Instead, we will break it into several sub-operations. For setting
	an array, we will send several commands. Each array will have its
	own GET_ARR_CMD and SET_ARR_CMD. Each will be followed by a
	sub-command, sent as a HEX encoded, terminated value.
	
	The SET_SEQ and GET_SEQ pairs work together with the sub-commands as follows.
	
	command		| subcommand			| signature		| meaning
	--------	|-----------------------|---------------|---------------------------------------------
	\b SET_SEQ	| SUBCMD_ARRAY_SIZE		| ()->(size)	| retrieves the *maximum* size of the array
	GET_SEQ		| SUBCMD_ARRAY_SIZE		| ()->(size)	| retrieves the *current* length of the array
	\b SET_SEQ	| SUBCMD_ARRAY_STARTING	| ---			| **is not used**
	GET_SEQ		| SUBCMD_ARRAY_STARTING	| ()->()		| tells the array function we are about to start getting the array
	\b SET_SEQ	| SUBCMD_ARRAY_ELEMENT	| (index,el)->() | sets an element at a given index location
	GET_SEQ		| SUBCMD_ARRAY_ELEMENT	| (index)->(el)	| retrieves an alement from a given index location
	\b SET_SEQ	| SUBCMD_ARRAY_FINISHED	| (length)->()	| sets the final length of the array. The receiver may perform some action too
	GET_SEQ		| SUBCMD_ARRAY_FINISHED	| ---			| **is not used**

	In the template definitions below, D is the class (ie FooClass) of the member
	function we want to	invoke and __target is the class instance pointer
	(ie ptr_to_a) we will invoke it on.
	*/

	//////////////////////////////////////////////////////////////////////////
	/// \name Sub-command constants
	/// \ingroup	HexProtocol 
	///@{

	const prot_cmd_t SUBCMD_ARRAY_SIZE = 0x01; ///< GET/SET subcommand (size) retrieves the maximum size of the array
	const prot_cmd_t SUBCMD_ARRAY_STARTING = 0x02; ///< GET subcommand () tells that we are about to receive the array
	const prot_cmd_t SUBCMD_ARRAY_ELEMENT = 0x03; ///< GET/SET subcommand (index, element) sets or gets an array element
	const prot_cmd_t SUBCMD_ARRAY_FINISHED = 0x04; ///< SET subcommand (length) finishes set or get and sets the total number of elements

	///@}
	//////////////////////////////////////////////////////////////////////////

	// Compile-time checks on the value sizes
	static_assert(sizeof(prot_long_t) == sizeof(long), "sizeof(prot_long_t) != sizeof(long)");
	static_assert(sizeof(prot_ulong_t) == sizeof(long), "sizeof(prot_ulong_t) != sizeof(long)");

#ifdef PROT_FLOAT_IEEE754
	static_assert(sizeof(prot_float_t) == sizeof(long), "sizeof(prot_float_t) != sizeof(long)");
#endif // #ifdef PROT_FLOAT_IEEE754



	/** Syntactic sugar for conditional chaining and short-circuit evaluation.

	\ingroup HexProtocol
	\code{.cpp}
		if (!test(doFirst() && doSecond()) {
			// failure clause
		} else {
			// success clause
		}
	\endcode
	 @param __chain boolean test chain to evaluate
	 @return Returns the value (result of) __chain
	 */
	inline bool test(bool __chain) {
		return __chain;
	}


	/////////////////////////////////////////////////////////////////////////
	// HexProtocolBase
	//

	/**	Protocol transmission class.
	
	\ingroup HexProtocol
	
	Drivers are expected to derive a class from HexProtocolBase
	and implement the indicated methods.
	
	@tparam DEV     sub-device that implements the Hex Protocol
	@tparam S       sub-device serial stream object or name
		@see [Curiously recurring template pattern]
		(https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
		for an explanation of why we need to include the derived
		type DEV as a template argument. Micromanager Devices make extensive use
		of this pattern
	*/

	template <class DEV, typename S>
	class HexProtocolBase {

	public:
		/////////////////////////////////////////////////////////////////////////
		/// \name Entry Point
		/// Used to start the protocol.
		///
		///@{

		/** Begin communication.
		@param __target will respond to serial requests via processXXX methods
		@param __stream implementation-dependent serial stream device or name */
		virtual void startProtocol(DEV* __target, S& __stream) {
			target_ = __target;
			stream_ = __stream;
			started_ = true;
		}

		/** End communication. Derived class may override. */
		virtual void endProtocol() {
			started_ = false;
		}

		/** Test whether startProtocol() was called. Derived class my override.
		\warning Derived classes should check hasStarted() before using
		the stream_ or target_ pointer. */
		virtual bool hasStarted() {
			return started_;
		};

		/** Default constructor that does nothing.
		Use startProtocol(target,stream) before any transactions. */
		HexProtocolBase() {}

		virtual ~HexProtocolBase() { }

		///@}
		/////////////////////////////////////////////////////////////////////////


	protected:
		/////////////////////////////////////////////////////////////////////////
		/// \name Common Implementation Methods, Lowest-level
		/// Each driver must create a derived class that implements these virtual 
		/// methods.
		///
		///@{

		/** Write a single byte */
		virtual bool writeByte(prot_byte_t __b) = 0;

		/** Write several bytes to the output. The term character should be included
		in the buffer, or you can use a single writeByte() to write
		the term character. */
		virtual size_t writeBuffer(const char* __buffer, size_t __size) = 0;

		/** Read a string of bytes from the input UNTIL a terminator character is received, or a
		timeout occurrs. The terminator character is NOT added to the end of the buffer. */
		virtual size_t readBufferUntilTerminator(char* __buffer, size_t __size, char __terminator) = 0;

		/** Reads a string of arbitrary length from the input UNTIL a terminator character is
		received, or a timeout occurrs. The terminator character is NOT added to the end
		of the string, but the string is null terminated.
		NOTE: On the Arduino, readStringUnreadStringUntilTerminatortil is much slower than
		readBufferUntilTerminator because each additional character might trigger a reallocation
		to increase the string buffer size. readStringUntilTerminator is primarily used for
		getValue<prot_string_t>(). */
		virtual size_t readStringUntilTerminator(prot_string_t& __str, char __terminator) = 0;

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Slave Implementation Methods, Lowest-level
		/// Only the slave driver(Arduino) must implement these virtual methods.
		/// The master (PC) does not receive single byte commands. It only sends them.
		///
		///@{

		/** Check to see if the input buffer has a byte to read. */
		virtual bool hasByte() {
			return false;
		}

		/** Read a single byte from the input buffer. */
		virtual bool readByte(prot_byte_t&) {
			return false;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Logging Methods, Lowest-level
		/// Host or slave drivers may implement and use these functions to, 
		/// say, reset a log or buffer. They are  not used by the code below, 
		/// but are driver-dependent.
		///
		///@{

		/** may be define by a device for locking a transaction */
		virtual void lockStream() { }

		/** may be define by a device for unlocking transaction */
		virtual void unlockStream() { }

	public:
		/** Helper class to use a local variable to automatically guard the start and end of a transaction.

		For example, declare HexProtocolBase::StreamGuard trans(pProto_); at the beginning
		of the function, which will call lockStream() upon creation and unlock()
		when the local variable goes out of scope.
		*/
		class StreamGuard {
		public:
			StreamGuard(HexProtocolBase* __pProto) : pProto_(__pProto) {
				pProto_->lockStream();
			}

			~StreamGuard() {
				pProto_->unlockStream();
			}

		protected:
			HexProtocolBase* pProto_;
		};

		///@}
		/////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////////////////////////////////////
		/// \name Receiving values, Low-level
		///
		/// Because this is a templated class, we cannot create template
		/// specializations for member functions. That gets flagged as
		/// partial specialization of a class.
		/// instead, we use a delagation trick. For each member function
		/// we need partial specialization, we create template structure
		/// with a "call" method that takes a "this" pointer and does the thing
		/// it is supposed to do.
		///
		/// Each getValueDelegate is a structure containing a single static
		/// "call" method.
		///
		///
		///@{

		/** Delegate to request a generic value. */
		template <class DD, typename SS, typename T>
		struct getValueDelegate {
			static bool call(HexProtocolBase<DD, SS>* __prot, T& __t) {
				static_assert(sizeof(T) <= sizeof(prot_ulong_t), "getValue does not work for this type");
				// store buffer on the stack
				char readBuf[PROT_HEX_BUFF_SIZE];
				size_t bytesRead = __prot->readBufferUntilTerminator(readBuf, PROT_HEX_BUFF_SIZE - 1, PROT_TERM_CHAR);
				if (bytesRead == 0) {
					return false;
				}
				// force null terminate the string for the conversion
				readBuf[bytesRead] = '\0';
				if (
					IS_SIGNED(T)) {
					/** HEX transfer of negative does not work well.
					We will handle negatives ourselves */
					char* negSign = strchr(readBuf, '-');
					if (negSign) {
						prot_long_t temp = strtoul(negSign + 1, nullptr, PROT_RADIX);
						__t = static_cast<T>(-temp);
					} else {
						prot_ulong_t temp = strtoul(readBuf, nullptr, PROT_RADIX);
						__t = static_cast<T>(temp);
					}
				} else {
					prot_ulong_t temp = strtoul(readBuf, nullptr, PROT_RADIX);
					__t = static_cast<T>(temp);
				}
				return true;
			}
		};

		/* Delegate to request a string value. Template specializations for getting arbitrary string values. */
		template <class DD, typename SS>
		struct getValueDelegate<DD, SS, prot_string_t> {
			static bool call(HexProtocolBase<DD, SS>* __prot, prot_string_t& __str) {
				size_t bytesRead = __prot->readStringUntilTerminator(__str, PROT_TERM_CHAR);
				return (bytesRead != 0);
			}
		};

		/** Delegate to request a float value. Template specializations for putting floating point values */
		template <class DD, typename SS>
		struct getValueDelegate<DD, SS, prot_float_t> {
			static bool call(HexProtocolBase<DD, SS>* __prot, prot_float_t& __val) {
#ifdef PROT_FLOAT_IEEE754
				/** The BIG assumption is that floats are IEEE-754 on both sides of the transfer. */
				prot_ulong_t temp;
				if (!__prot->template getValue<prot_ulong_t>(temp)) {
					return false;
				}
				__val = *reinterpret_cast<prot_float_t*>(&temp);
				return true;
#else // NOT #ifdef PROT_FLOAT_IEEE754



				// store buffer on the stack
				char buf[PROT_FLOAT_BUFF_SIZE ];
				size_t bytesRead = __prot->readBufferUntilTerminator(buf, PROT_FLOAT_BUFF_SIZE - 1, PROT_TERM_CHAR);
				if (bytesRead == 0) {
					return false;
				}
				// force null terminate the string for the conversion
				buf[bytesRead] = '\0';
				__val = static_cast<prot_float_t>(prot_strtof(buf));
				return true;
#endif // #ifdef PROT_FLOAT_IEEE754



			}
		};

		/** Delegate to request a double value. Template specializations for getting double values */
		template <class DD, typename SS>
		struct getValueDelegate<DD, SS, double> {
			static bool call(HexProtocolBase<DD, SS>* __prot, double& __val) {
				prot_float_t temp;
				bool ret = __prot->template getValue<prot_float_t>(temp);
				__val = temp;
				return ret;
			}
		};


		/** Get a generic value. Calls the correct Delegate. */
		template <typename T>
		bool getValue(T& __t) {
			return getValueDelegate<DEV, S, T>::call(this, __t);
		}

		/** Request a string buffer explicitely. */
		bool getString(char* __strbuf, size_t __size) {
			size_t bytesRead = readBufferUntilTerminator(__strbuf, __size, PROT_TERM_CHAR);
			// force null terminate the string
			if (bytesRead < __size - 1) {
				__strbuf[bytesRead] = '\0';
			}
			return (bytesRead != 0);
		}

		///@}
		/////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////////////////////////////////////
		/// \name Sending values, Low-level
		///
		/// Because this is a templated class, we cannot create template
		/// specializations for member functions. That gets flagged as
		/// partial specialization of a class.
		/// instead, we use a delagation trick. For each member function
		/// we need partial specialization, we create template structure
		/// with a "call" method that takes a "this" pointer and does the thing
		/// it is supposed to do.
		///
		/// Each putValueDelegate is a structure containing a single static
		/// "call" method.
		///
		///@{

		/** Delegate to send a generic value. */
		template <class DD, typename SS, typename T>
		struct putValueDelegate {
			static bool call(HexProtocolBase<DD, SS>* __prot, T __val) {
				static_assert(sizeof(T) <= sizeof(prot_ulong_t), "putValue does not work for this type");
				// store buffer on the stack
				char writeBuf[PROT_HEX_BUFF_SIZE];
				if (
					IS_SIGNED(T)) {
					prot_long_t temp = static_cast<prot_long_t>(__val);
					if (temp < 0) {
						/** HEX transfer of negative does not work well.
						We will handle negatives ourselves */
						writeBuf[0] = '-';
						prot_ultohexstr(-temp, writeBuf + 1, PROT_RADIX);
					} else {
						prot_ultohexstr(temp, writeBuf, PROT_RADIX);
					}
				} else {
					prot_ulong_t temp = static_cast<prot_ulong_t>(__val);
					prot_ultohexstr(temp, writeBuf, PROT_RADIX);
				}
				size_t len = strlen(writeBuf);
				writeBuf[len++] = PROT_TERM_CHAR ;
				size_t bytesWritten = __prot->writeBuffer(writeBuf, len);
				return (bytesWritten == len);
			}
		};

		/* Delegate to send a string value. Template specializations for putting arbitrary string values*/
		template <class DD, typename SS>
		struct putValueDelegate<DD, SS, prot_string_t> {
			static bool call(HexProtocolBase<DD, SS>* __prot, const prot_string_t __str) {
				return __prot->putString(__str.c_str());
			}
		};

		/* Delegate to send a char* string value. Template specializations for putting arbitrary string buffer values*/
		template <class DD, typename SS>
		struct putValueDelegate<DD, SS, const char*> {
			static bool call(HexProtocolBase<DD, SS>* __prot, const char* __strbuf) {
				return __prot->putString(__strbuf);
			}
		};

		/** Delegate to send a float value. Template specializations for getting floating point values */
		template <class DD, typename SS>
		struct putValueDelegate<DD, SS, prot_float_t> {
			static bool call(HexProtocolBase<DD, SS>* __prot, prot_float_t __val) {
#ifdef PROT_FLOAT_IEEE754
				/** The BIG assumption is that floats are IEEE-754 on both sides of the transfer. */
				prot_ulong_t temp = *reinterpret_cast<prot_ulong_t*>(&__val);
				return __prot->template putValue<prot_ulong_t>(temp);
#else // NOT #ifdef PROT_FLOAT_IEEE754
				// store buffer on the stack
				char buf[PROT_FLOAT_BUFF_SIZE ];
				prot_ftostr(__val, buf, PROT_FLOAT_BUFF_SIZE, PROT_FLOAT_MAX_PREC);
				size_t len = strlen(buf);
				buf[len++] = PROT_TERM_CHAR ;
				size_t bytesWritten = __prot->writeBuffer(buf, len);
				return (bytesWritten == len);
#endif // #ifdef PROT_FLOAT_IEEE754
			}
		};

		/** Delegate to send a double value. Template specializations for putting double values */
		template <class DD, typename SS>
		struct putValueDelegate<DD, SS, double> {
			static bool call(HexProtocolBase<DD, SS>* __prot, double __val) {
				return __prot->template putValue<prot_float_t>(static_cast<prot_float_t>(__val));
			}
		};

		/** Send a generic value. Calls the correct Delegate. */
		template <typename T>
		bool putValue(const T __val) {
			return putValueDelegate<DEV, S, T>::call(this, __val);
		}

		/** Send a string buffer explicitely. */
		bool putString(const char* __str) {
			size_t len = strlen(__str);
			size_t bytesWritten = len > 0 ? writeBuffer(__str, len) : 0;
			char term = PROT_TERM_CHAR;
			bytesWritten += writeBuffer(&term, 1);
			return (bytesWritten == len + 1);
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Sending and receiving commands, Mid-level
		///
		///@{

		/** Write a single byte to the output */
		bool putCommand(prot_cmd_t __cmd) {
			return writeByte(static_cast<prot_byte_t>(__cmd));
		}

		/** Write a single byte to the output followed by a channel number*/
		bool putChannelCommand(prot_cmd_t __cmd, prot_chan_t __c) {
			return test(putCommand(__cmd) && putValue<prot_chan_t>(__c));
		}

		/** Send encoded reply to the output */
		bool reply(prot_cmd_t __cmd) {
			return putValue<prot_cmd_t>(__cmd);
		}

		/** Send encoded reply of PROT_ERROR to the output.
		ALWAYS returns false, so you functions may return replyError() straight away */
		bool replyError() {
			putValue<prot_cmd_t>(PROT_ERROR);
			return false;
		}

		/** was the reply good? */
		bool checkReply(prot_cmd_t __cmd) {
			prot_cmd_t answer = 0;
			return test(getValue<prot_cmd_t>(answer) && (answer == __cmd));
		}

		/** Determine whether a single byte is in the read buffer */
		bool hasCommand() {
			return hasByte();
		}


		/** get a single byte command from the read buffer */
		prot_cmd_t getCommand() {
			prot_byte_t b;
			if (readByte(b)) {
				return static_cast<prot_cmd_t>(b);
			}
			return PROT_ERROR ;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Command dispatching (sending), High-level
		///
		///@{

		/** Dispatch a task command. */
		bool dispatchTask(prot_cmd_t __cmdTask) {
			return test(putCommand(__cmdTask) && checkReply(__cmdTask));
		}

		/** Dispatch a get single value command. */
		template <typename T>
		bool dispatchGet(prot_cmd_t __cmdGet, T& __t) {
			return test(putCommand(__cmdGet) && checkReply(__cmdGet) && getValue<T>(__t));
		}

		/** Dispatch a get two values command. */
		template <typename T, typename U>
		bool dispatchGet(prot_cmd_t __cmdGet, T& __t, U& __u) {
			return test(putCommand(__cmdGet) && checkReply(__cmdGet) && getValue<T>(__t) && getValue<U>(__u));
		}

		/** Dispatch a get string command. */
		bool dispatchGetString(prot_cmd_t __cmdSet, char* __strbuf, size_t __size) {
			return test(putCommand(__cmdSet) && getString(__strbuf, __size) && checkReply(__cmdSet));
		}

		/** Dispatch a set single value command. */
		template <typename T>
		bool dispatchSet(prot_cmd_t __cmdSet, const T __t) {
			return test(putCommand(__cmdSet) && putValue<T>(__t) && checkReply(__cmdSet));
		}

		/** Dispatch a set two values command. */
		template <typename T, typename U>
		bool dispatchSet(prot_cmd_t __cmdSet, const T __t, const U __u) {
			return test(putCommand(__cmdSet) && putValue<T>(__t) && putValue<U>(__u) && checkReply(__cmdSet));
		}

		/** Dispatch a set string command. */
		bool dispatchSetString(prot_cmd_t __cmdSet, const char* __str) {
			return test(putCommand(__cmdSet) && putString(__str) && checkReply(__cmdSet));
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Channel Command dispatching (sending), High-level
		//
		/// All commands are immediately followed by a channel number
		///
		///@{

		/** Dispatch a task command to a specific channel. */
		bool dispatchChannelTask(prot_cmd_t __cmdTask, prot_chan_t __chan) {
			return test(putChannelCommand(__cmdTask, __chan) && checkReply(__cmdTask));
		}

		/** Dispatch a get single value command to a specific channel. */
		template <typename T>
		bool dispatchChannelGet(prot_cmd_t __cmdGet, prot_chan_t __chan, T& __t) {
			return test(putChannelCommand(__cmdGet, __chan) && checkReply(__cmdGet) && getValue<T>(__t));
		}

		/** Dispatch a get two values command to a specific channel. */
		template <typename T, typename U>
		bool dispatchChannelGet(prot_cmd_t __cmdGet, prot_chan_t __chan, T& __t, U& __u) {
			return test(putChannelCommand(__cmdGet, __chan) && checkReply(__cmdGet) && getValue<T>(__t) && getValue<U>(__u));
		}

		/** Dispatch a get string command to a specific channel. */
		bool dispatchChannelGetString(prot_cmd_t __cmdSet, prot_chan_t __chan, char* __strbuf, size_t __size) {
			return test(putChannelCommand(__cmdSet, __chan) && getString(__strbuf, __size) && checkReply(__cmdSet));
		}

		/** Dispatch a set single value command to a specific channel. */
		template <typename T>
		bool dispatchChannelSet(prot_cmd_t __cmdSet, prot_chan_t __chan, const T __t) {
			return test(putChannelCommand(__cmdSet, __chan) && putValue<T>(__t) && checkReply(__cmdSet));
		}

		/** Dispatch a set two values command to a specific channel. */
		template <typename T, typename U>
		bool dispatchChannelSet(prot_cmd_t __cmdSet, prot_chan_t __chan, const T __t, const U __u) {
			return test(putChannelCommand(__cmdSet, __chan) && putValue<T>(__t) && putValue<U>(__u) && checkReply(__cmdSet));
		}

		/** Dispatch a get string command to a specific channel. */
		bool dispatchChannelSetString(prot_cmd_t __cmdSet, prot_chan_t __chan, const char* __str) {
			return test(putChannelCommand(__cmdSet, __chan) && putString(__str) && checkReply(__cmdSet));
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Array Command dispatching (sending), High-level
		///
		///@{

		/** Send an array of values, one at a time. 
		pseudocode (without the protocol checks)
		\code{.cpp}
			// get the maximum size of the array
			put(SET, SUBCMD_ARRAY_SIZE)
			getValue(maxSize)
			// check that the array will fit
			check size <= maxSize
			// send the elements
			for (i=0; i<size; i++) {
				putCommand(SET, SUBCMD_ARRAY_ELEMENT)
				putValue(i)
				putValue(element[i])
			}
			// send the finished marker
			putCommand(SET, SUBCMD_ARRAY_FINISHED)
			putValue(size)
		\endcode
		*/
		template <typename T>
		bool dispatchSetArray(prot_cmd_t __cmdSet, const T* __pt, prot_size_t __size) {
			// get the maximum size of the remote array
			prot_size_t maxSize;
			if (!test(putCommand(__cmdSet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdSet) && getValue(maxSize) && __size <= maxSize)) {
				return false;
			}
			// Set the elements
			for (prot_size_t i = 0; i < __size; i++) {
				if (!test(putCommand(__cmdSet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_ELEMENT)
					&& putValue(i) && putValue(__pt[i]) && checkReply(__cmdSet))) {
					return false;
				}
			}

			// Finalize by setting the length
			if (!test(putCommand(__cmdSet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_FINISHED)
				&& putValue(__size) && checkReply(__cmdSet))) {
				return false;
			}
			return true;
		}

		/** Request the maximum size of the receive array buffer. */
		bool dispatchGetArrayMaxSize(prot_cmd_t __cmdSet, prot_size_t& __maxSize) {
			// get the max size of the array
			return test(putCommand(__cmdSet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdSet) && getValue(__maxSize));
		}

		/** Request the curent array length. */
		bool dispatchGetArraySize(prot_cmd_t __cmdGet, prot_size_t& __size) {
			// get the current size of the array
			return test(putCommand(__cmdGet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdGet) && getValue(__size));
		}

		/** Request an array of values, one at a time. 
		pseudocode (without the protocol checks)
		\code{.cpp}
			// send the start get marker
			put(GET, SUBCMD_ARRAY_STARTING)
			// get the current array size
			put(GET, SUBCMD_ARRAY_SIZE)
			getValue(size)
			// check that the array will fit in our receiving buffer
			check size <= maxSize
			// retrieve the elements
			for (i=0; i<size; i++) {
				put(GET, SUBCMD_ARRAY_ELEMENT)
				putValue(i)
				getValue(element[i])
			}
		\endcode
		*/
		template <typename T>
		bool dispatchGetArray(prot_cmd_t __cmdGet, T* __pt, prot_size_t __maxSize, prot_size_t& __size) {
			// tell the array that we are about to getting it
			if (!test(putCommand(__cmdGet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_STARTING) && checkReply(__cmdGet))) {
				return false;
			}
			// get the current size of the array
			if (!test(putCommand(__cmdGet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdGet) && getValue(__size) && __size <= __maxSize
			)) {
				return false;
			}
			// Get the elements
			for (prot_size_t i = 0; i < __size; i++) {
				if (!test(putCommand(__cmdGet) && putValue<prot_cmd_t>(SUBCMD_ARRAY_ELEMENT)
					&& putValue(i) && checkReply(__cmdGet) && getValue(__pt[i]))) {
					return false;
				}
			}
			return true;
		}

		/////////////////////////////////////////////////////////////////////////
		/// \name Channel Array Command dispatching (sending), High-level
		///
		///@{

		template <typename T>
		bool dispatchChannelSetArray(prot_cmd_t __cmdSet, prot_chan_t __chan, const T* __pt, prot_size_t __size) {
			// get the maximum size of the remote array
			prot_size_t maxSize;
			if (!test(putChannelCommand(__cmdSet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdSet) && getValue(maxSize) && __size <= maxSize)) {
				return false;
			}
			// Set the elements
			for (prot_size_t i = 0; i < __size; i++) {
				if (!test(putChannelCommand(__cmdSet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_ELEMENT)
					&& putValue(i) && putValue(__pt[i]) && checkReply(__cmdSet))) {
					return false;
				}
			}

			// Finalize by setting the length
			if (!test(putChannelCommand(__cmdSet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_FINISHED)
				&& putValue(__size) && checkReply(__cmdSet))) {
				return false;
			}
			return true;
		}

		/** Request the maximum size of the receive array buffer. */
		bool dispatchChannelGetArrayMaxSize(prot_cmd_t __cmdSet, prot_chan_t __chan, prot_size_t& __maxSize) {
			// get the max size of the array
			return test(putChannelCommand(__cmdSet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdSet) && getValue(__maxSize));
		}

		/** Request the curent array length. */
		bool dispatchChannelGetArraySize(prot_cmd_t __cmdGet, prot_chan_t __chan, prot_size_t& __size) {
			// get the current size of the array
			return test(putChannelCommand(__cmdGet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdGet) && getValue(__size));
		}

		template <typename T>
		bool dispatchChannelGetArray(prot_cmd_t __cmdGet, prot_chan_t __chan, T* __pt, prot_size_t __maxSize, prot_size_t& __size) {
			// tell the array that we are about to getting it
			if (!test(putChannelCommand(__cmdGet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_STARTING) && checkReply(__cmdGet))) {
				return false;
			}
			// get the current size of the array
			if (!test(putChannelCommand(__cmdGet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_SIZE)
				&& checkReply(__cmdGet) && getValue(__size) && __size <= __maxSize
				)) {
				return false;
			}
			// Get the elements
			for (prot_size_t i = 0; i < __size; i++) {
				if (!test(putChannelCommand(__cmdGet, __chan) && putValue<prot_cmd_t>(SUBCMD_ARRAY_ELEMENT)
					&& putValue(i) && checkReply(__cmdGet) && getValue(__pt[i]))) {
					return false;
				}
			}
			return true;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Command handling (receiving), High-level
		///
		/// Command handlers use a pre-defined DEV class method that 
		/// will process deserialized values for the target_ set by
		/// startProtocol(target, stream))
		///
		/// \note We cannot typdef templated functions directly, but we
		/// can create a template struct contained a typdef function type.
		/// So, we place each Function prototype inside of a structure
		/// with the common typedef function prototype of 'type'
		///
		///@{

		//-----------------------------------------------------------------------
		// process commands
		//-----------------------------------------------------------------------

		/** Member function that processes any type of command. The simplest version would simply be a
		big switch(__cmd) which calls processTask(), processSet(), etc based on the __cmd. */
		struct CommandFn {
			typedef void (DEV::*type)(prot_cmd_t __cmd);
		};

		/** A single entry point for command handling. Calls a ProcessCommandFn. */
		void processCommand(prot_cmd_t __cmd, typename CommandFn::type __processFn) {
			if (target_) {
				(target_ ->* __processFn)(__cmd);
			}
		};

		//-----------------------------------------------------------------------
		// process tasks
		//-----------------------------------------------------------------------

		/** Member function that processes a task command.
		@return Must return true if successful
		*/
		struct TaskFn {
			typedef bool (DEV::*type)(void);
		};

		/** Process a task command. Calls a TaskFn. */
		bool processTask(prot_cmd_t __cmdTask, typename TaskFn::type __taskFn) {
			if (test(target_ && (target_ ->* __taskFn)())) {
				return reply(__cmdTask);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel tasks
		//-----------------------------------------------------------------------

		/** Member function that processes a task command on a specific channel
		@return Must return true if successful
		*/
		struct ChannelTaskFn {
			typedef bool (DEV::*type)(prot_chan_t __chan);
		};

		/** Process a task command on a specific channel. Calls a ChannelTaskFn. */
		bool processChannelTask(prot_cmd_t __cmdTask, typename ChannelTaskFn::type __taskFn) {
			prot_chan_t chan;
			if (test(getValue<prot_chan_t>(chan) && target_ && (target_->*__taskFn)(chan))) {
				return reply(__cmdTask);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process set single value
		//-----------------------------------------------------------------------

		/** Simple processSet that does not use a delegate to do something 
		with that value. 
		Takes an optional __afterSet task function that will be called 
		and checked after the value is set. */
		template <typename T>
		bool processSet(prot_cmd_t __cmdSet, T& __val, typename TaskFn::type __afterSet = 0) {
			if (getValue<T>(__val)) {
				if (__afterSet) {
					if (!test(target_ && (target_ ->* __afterSet)())) {
						return replyError();
					}
				}
				return reply(__cmdSet);
			}
			return replyError();
		}

		/** Member function that processes a set value command.
		@return Must return true if successful
		*/
		template <typename T>
		struct SetValueFn {
			typedef bool (DEV::*type)(const T __t);
		};

		/** Process a set value command. Calls a SetValueFn. */
		template <typename T>
		bool processSet(prot_cmd_t __cmdSet, typename SetValueFn<T>::type __setFn) {
			T t_val;
			if (test(getValue<T>(t_val) && target_ && (target_ ->* __setFn)(t_val))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel set single value
		//-----------------------------------------------------------------------

		/** Member function that processes a set value command on a specific channel.
		@return Must return true if successful
		*/
		template <typename T>
		struct ChannelSetValueFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, const T __t);
		};

		/** Process a set value command on a specific channel. Calls a ChannelSetValueFn. */
		template <typename T>
		bool processChannelSet(prot_cmd_t __cmdSet, typename ChannelSetValueFn<T>::type __setFn) {
			prot_chan_t chan;
			T t_val;
			if (test(getValue<prot_chan_t>(chan) && getValue<T>(t_val) && target_ && (target_->*__setFn)(chan, t_val))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process set two values
		//-----------------------------------------------------------------------

		/** Simple processSet two value that does not use a delegate to 
		do something with that values. 
		Takes an optional __afterSet task function that will be called 
		and checked after the value is set. */
		template <typename T, typename U>
		bool processSet(prot_cmd_t __cmdSet, T& __t_val, U& __u_val, typename TaskFn::type __afterSet = 0) {
			if (test(getValue<T>(__t_val) && getValue<U>(__u_val))) {
				if (__afterSet) {
					if (!test(target_ && (target_ ->* __afterSet)())) {
						return replyError();
					}
				}
				return reply(__cmdSet);
			}
			return replyError();
		}

		/** Member function that processes a set two values command.
		@return Must return true if successful
		*/
		template <typename T, typename U>
		struct SetTwoValuesFn {
			typedef bool (DEV::*type)(const T __t, const U __u);
		};

		/** Process a set two values command. Calls a SetTwoValuesFn. */
		template <typename T, typename U>
		bool processSet(prot_cmd_t __cmdSet, typename SetTwoValuesFn<T, U>::type __setFn) {
			T t_val;
			U u_val;
			if (test(getValue<T>(t_val) && getValue<U>(u_val) && target_ && (target_ ->* __setFn)(t_val, u_val))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel set two values
		//-----------------------------------------------------------------------

		/** Member function that processes a set two values command on a specific channel.
		@return Must return true if successful
		*/
		template <typename T, typename U>
		struct ChannelSetTwoValuesFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, const T __t, const U __u);
		};

		/** Process a set two values command on a specific channel. Calls a ChannelSetTwoValuesFn. */
		template <typename T, typename U>
		bool processChannelSet(prot_cmd_t __cmdSet, typename ChannelSetTwoValuesFn<T, U>::type __setFn) {
			prot_chan_t chan;
			T t_val;
			U u_val;
			if (test(getValue<prot_chan_t>(chan) && getValue<T>(t_val) && getValue<U>(u_val) && target_ && (target_->*__setFn)(chan, t_val, u_val))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process get single value
		//-----------------------------------------------------------------------

		/** Simple processGet that does not use a delegate. 
		Takes an optional __beforeGet task function that will be called 
		and checked before the value is retrieved. */
		template <typename T>
		bool processGet(prot_cmd_t __cmdGet, T __val, typename TaskFn::type __beforeGet = 0) {
			if (__beforeGet) {
				if (!test(target_ && (target_ ->* __beforeGet)())) {
					replyError();
				}
			}
			return test(reply(__cmdGet) && putValue<T>(__val));
		}

		/** Member function that processes a get value command.
		@return Must return true if successful
		*/
		template <typename T>
		struct GetValueFn {
			typedef bool (DEV::*type)(T& __t);
		};

		/** Process a get value command. Calls a GetValueFn. */
		template <typename T>
		bool processGet(prot_cmd_t __cmdGet, typename GetValueFn<T>::type __getFn) {
			T t_val;
			if (test(target_ && (target_ ->* __getFn)(t_val))) {
				return test(reply(__cmdGet) && putValue<T>(t_val));
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel get single value
		//-----------------------------------------------------------------------

		/** Member function that processes a get value command on a specific channel.
		@return Must return true if successful
		*/
		template <typename T>
		struct ChannelGetValueFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, T& __t);
		};

		/** Process a get value command on a specific channel. Calls a ChannelGetValueFn. */
		template <typename T>
		bool processChannelGet(prot_cmd_t __cmdGet, typename ChannelGetValueFn<T>::type __getFn) {
			prot_chan_t chan;
			T t_val;
			if (test(getValue<prot_chan_t>(chan) && target_ && (target_->*__getFn)(chan, t_val))) {
				return test(reply(__cmdGet) && putValue<T>(t_val));
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process get two values
		//-----------------------------------------------------------------------

		/** Simple processGet two values that does not use a delegate. 
		Takes an optional __beforeGet task function that will be called 
		and checked before the value is retrieved. */
		template <typename T, typename U>
		bool processGet(prot_cmd_t __cmdGet, T __t_val, U __u_val, typename TaskFn::type __beforeGet = 0) {
			if (__beforeGet) {
				if (!test(target_ && (target_ ->* __beforeGet)())) {
					replyError();
				}
			}
			return test(reply(__cmdGet) && putValue<T>(__t_val) && putValue<U>(__u_val));
		}

		/** Member function that procces a get two values command.
		@return Must return true if successful
		*/
		template <typename T, typename U>
		struct GetTwoValuesFn {
			typedef bool (DEV::*value)(T& __t, U& __u);
		};

		/** Process a get two values command. Calls a GetTwoValuesFn. */
		template <typename T, typename U>
		bool processGet(prot_cmd_t __cmdGet, typename GetTwoValuesFn<T, U>::value __getFn) {
			T t_val;
			U u_val;
			if (test(target_ && (target_ ->* __getFn)(t_val, u_val))) {
				return test(reply(__cmdGet) && putValue<T>(t_val) && putValue<U>(u_val));
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel get two values
		//-----------------------------------------------------------------------

		/** Member function that procces a get two values command on a specific channel.
		@return Must return true if successful
		*/
		template <typename T, typename U>
		struct ChannelGetTwoValuesFn {
			typedef bool (DEV::*value)(prot_chan_t __chan, T& __t, U& __u);
		};

		/** Process a get two values command on a specific channel. Calls a ChannelGetTwoValuesFn. */
		template <typename T, typename U>
		bool processChannelGet(prot_cmd_t __cmdGet, typename GetTwoValuesFn<T, U>::value __getFn) {
			prot_chan_t chan;
			T t_val;
			U u_val;
			if (test(getValue<prot_chan_t>(chan) && target_ && (target_->*__getFn)(t_val, u_val))) {
				return test(reply(__cmdGet) && putValue<T>(t_val) && putValue<U>(u_val));
			}
			return replyError();
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name String buffer Command handling (receiving), High-level
		///
		///@{

		//-----------------------------------------------------------------------
		// process set string
		//-----------------------------------------------------------------------

		/** Simple processSetString that does not use a delegate get the string
		buffer information.
		Takes an optional __afterSet task function that will be called
		and checked after the value is set. */
		bool processSetString(prot_cmd_t __cmdSet, char* __strbuf, size_t __size, typename TaskFn::type __afterSet = 0) {
			if (getString(__strbuf, __size)) {
				if (__afterSet) {
					if (!test(target_ && (target_ ->* __afterSet)())) {
						return replyError();
					}
				}
				return reply(__cmdSet);
			}
			return replyError();
		}

		/** Member function that processes a set string buffer requests. 

		Before filling string buffer, the processSetString function requests 
	    a pointer to the buffer (__strbuf) and the maximum size of the 
	    buffer (__maxSize) with the __finalSize set to zero. SetStringBufferFn
	    must respond by filling out __strbuf and __maxSize. 
	    Once the string has been filled, the processSetString function 
	    calls the SetStringFn with the __finalSize argument set to the size 
	    of the received string. The SetStringFn might do something with
	    the received string or just return true.

		@param[out] __strbuf pointer to the string buffer to fill
		@param[out] __maxSize maximum size of the buffer to fill
		@param[in]  __finalSize size of the string after filling.
		@return Must return true if successful
		*/
		struct SetStringFn {
			typedef bool (DEV::*type)(char*& __strbuf, size_t& __maxSize, size_t __finalSize);
		};

		/** Process a set-string command for a char* string buffer. Calls a SetStringFn. */
		bool processSetString(prot_cmd_t __cmdSet, typename SetStringFn::type __strbufFn) {
			size_t maxSize;
			char* strbuf;
			bool goodbuf = test(target_ && (target_ ->* __strbufFn)(strbuf, maxSize, 0));

			if (test(goodbuf && getString(strbuf, maxSize) && target_ && (target_ ->* __strbufFn)(strbuf, maxSize, strlen(strbuf)))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel set string
		//-----------------------------------------------------------------------

		/** Member function that processes a set string buffer requests on a specific channel.
		@see SetStringFn
		*/
		struct ChannelSetStringFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, char*& __strbuf, size_t& __maxSize, size_t __finalSize);
		};

		/** Process a set-string command for a char* string buffer on a specific channel. Calls a ChannelSetStringFn. */
		bool processChannelSetString(prot_cmd_t __cmdSet, typename ChannelSetStringFn::type __strbufFn) {
			prot_chan_t chan;
			size_t maxSize;
			char* strbuf;
			bool goodbuf = test(getValue<prot_chan_t>(chan) && target_ && (target_->*__strbufFn)(chan, strbuf, maxSize, 0));

			if (test(goodbuf && getString(strbuf, maxSize) && target_ && (target_->*__strbufFn)(chan, strbuf, maxSize, strlen(strbuf)))) {
				return reply(__cmdSet);
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process get string
		//-----------------------------------------------------------------------

		/** Simple processGetString that does not use a delegate to get the
		string buffer information.
		Takes an optional __beforeGet task function that will be called
		and checked before the value is retrieved. */
		bool processGetString(prot_cmd_t __cmdGet, const char* __str, typename TaskFn::type __beforeGet = 0) {
			if (__beforeGet) {
				if (!test(target_ && (target_ ->* __beforeGet)())) {
					replyError();
				}
			}
			return test(reply(__cmdGet) && putString(__str));
		}

		/** Member function that processes get string requests. Must return
		a pointer to the string buffer.
		@param[out] __strbuf pointer to the string to get
		@return Must return true if successful
		*/
		struct GetStringFn {
			typedef bool (DEV::*type)(const char*& __strbuf);
		};

		/** Process a get string command. Calls a GetStringFn. */
		bool processGetString(prot_cmd_t __cmdGet, typename GetStringFn::type __strbufFn) {
			const char* strbuf;
			if (test(target_ && (target_ ->* __strbufFn)(strbuf))) {
				return test(reply(__cmdGet) && putString(strbuf));
			}
			return replyError();
		}

		//-----------------------------------------------------------------------
		// process channel get string
		//-----------------------------------------------------------------------

		/** Member function that processes get string requests on a specific channel.
		@see GetStringFn
		*/
		struct ChannelGetStringFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, const char*& __strbuf);
		};

		/** Process a get string command on a specific channel. Calls a ChannelGetStringFn. */
		bool processChannelGetString(prot_cmd_t __cmdGet, typename ChannelGetStringFn::type __strbufFn) {
			prot_chan_t chan;
			const char* strbuf;
			if (test(getValue<prot_chan_t>(chan) && target_ && (target_->*__strbufFn)(chan, strbuf))) {
				return test(reply(__cmdGet) && putString(strbuf));
			}
			return replyError();
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Array Command handling (sending & receiving), High-level
		///
		///@{

		//-----------------------------------------------------------------------
		// process set set array
		//-----------------------------------------------------------------------

		/** Simple processSetArray that does not use a delegate to get
		the array information.
		Takes an optional __afterSet task function that will be called
		and checked after the value is set. */
		template <typename T>
		bool processSetArray(prot_cmd_t __cmdSet, T* __pArr, size_t __maxSize, size_t& __finalSize, typename TaskFn::type __afterSet = 0) {
			int subCmd;
			if (!getValue<int>(subCmd)) {
				return replyError();
			}
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				return test(reply(__cmdSet) && putValue(__maxSize));
			} else if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				T el;
				if (test(getValue(index) && getValue(el) && index < __maxSize)) {
					__pArr[index] = el;
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			} else if (subCmd == SUBCMD_ARRAY_FINISHED) {
				if (getValue(__finalSize)) {
					if (__afterSet) {
						if (!test(target_ && (target_ ->* __afterSet)())) {
							return replyError();
						}
					}
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			}
			return replyError();
		}

		/** Member function that processes set array requests. 
		
		Before filling the array, the processSetArray function sets __maxSize=0
		and __finalSize=0 to request a pointer to the buffer (__pArr) and 
		the maximum size of the buffer (__maxSize). SetArrayFn must respond	by filling out __pArr and __maxSize.

		Before starting to fill the array, processSetArray notifies the SetArrayFn by
		setting __maxSize = previous maximum size and __finalSize=0. The SetArrayFn
		does not need to do anything but respond with true. processSetArray only
		calls the SetArrayFn once, on the first index (=0).
		
		Once the array has been filled,	the processSetArray function calls 
		SetArrayFn with the __finalSize argument > 0.

		@param[out] __pArr			OUT: pointer to the array to fill
		@param[in,out] __maxSize	IN: 0 to request max size, >0 to indicated filling. 
									OUT: maximum size of the array to fill
		@param[in]	__finalSize size of the array after filling.
			\warning Your SetArrayFn will be called multiple times with __finalSize=0
			to get the maximum array size. __Do not__ set your array's length __until__
			you see a __finalSize > 0 value. Otherwise, your array's length
			will get reset on every request for __maxSize!
		@return Must return true if successful
		*/
		template <typename T>
		struct SetArrayFn {
			typedef bool (DEV::*type)(T*& __pArr, size_t& __maxSize, size_t __finalSize);

		};

		/** Process a set array command. Calls a SetArrayFn. */
		template <typename T>
		bool processSetArray(prot_cmd_t __cmdSet, typename SetArrayFn<T>::type __arrFn) {
			prot_size_t maxSize;
			T* pArr;
			maxSize = 0;
			bool goodArray = test(target_ && (target_ ->* __arrFn)(pArr, maxSize, 0));

			int subCmd;
			if (!getValue<int>(subCmd)) {
				return replyError();
			}
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				if (goodArray) {
					return test(reply(__cmdSet) && putValue(maxSize));
				} else {
					return replyError();
				}
			} else if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				T el;
				if (test(getValue(index) && getValue(el) && index < maxSize && goodArray)) {
					if (index == 0) {
						if (!test(target_ && (target_->*__arrFn)(pArr, maxSize, 0))) {
							return replyError();
						}
					}
					pArr[index] = el;
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			} else if (subCmd == SUBCMD_ARRAY_FINISHED) {
				prot_size_t finalSize;
				if (getValue(finalSize)) {
					if (goodArray) {
						if (!test(target_ && (target_ ->* __arrFn)(pArr, maxSize, finalSize))) {
							return replyError();
						}
					}
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			}
			return replyError();
		}


		//-----------------------------------------------------------------------
		// process get array
		//-----------------------------------------------------------------------

		/** Simple processGetArray that does not use a delegate to get the
		array information.
		Takes an optional __beforeGet task function that will be called
		and checked before the value is retrieved. */
		template <typename T>
		bool processGetArray(prot_cmd_t __cmdGet, T* __pArr, size_t __size, typename TaskFn::type __beforeGet = 0) {
			prot_cmd_t subCmd;
			if (!getValue(subCmd)) {
				return replyError();
			}
			if (subCmd == SUBCMD_ARRAY_STARTING) {
				if (__beforeGet) {
					if (!test(target_ && (target_ ->* __beforeGet)())) {
						return replyError();
					}
				}
				return reply(__cmdGet);
			}
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				return test(reply(__cmdGet) && putValue(__size));
			}
			if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				if (test(getValue(index) && index < __size)) {
					T el = __pArr[index];
					return test(reply(__cmdGet) && putValue(el));
				} else {
					return replyError();
				}
			}
			return replyError();
		}

		/** Member function that processes get array requests. Must return
		a pointer to the array buffer (__pArr) and the array buffer length
		(__finalSize).
	
		@param[in] __beforeGetFlag true if getArray is starting, false otherwise
		@param[out] __pArr pointer to the array to get
		@param[out]	__finalSize size of the array to get.
		@return Must return true if successful
		*/
		template <typename T>
		struct GetArrayFn {
			typedef bool (DEV::*type)(bool __beforeGetFlag, T*& __pArr, prot_size_t& __size);
		};

		/** Process a get array command. Calls a GetArrayFn. */
		template <typename T>
		bool processGetArray(prot_cmd_t __cmdGet, typename GetArrayFn<T>::type __arrFn) {
			prot_cmd_t subCmd;
			if (!getValue(subCmd)) {
				// reply(0xB1); // Debug point
				return replyError();
			}
			prot_size_t size;
			T* pArr;
			if (subCmd == SUBCMD_ARRAY_STARTING) {
				// tell the GetArrayFn we are starting. Allows beforeGet processing
				if (!test(target_ && (target_ ->* __arrFn)(true, pArr, size))) {
					return replyError();
				}
				return reply(__cmdGet);
			}
			// we are in the middle of getting. Call GetArrayFn with beforeGet = false
			bool goodArray = target_ && (target_ ->* __arrFn)(false, pArr, size);
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				if (goodArray) {
					return test(reply(__cmdGet) && putValue(size));
				} else {
					// reply(0xB2); // Debug point
					return replyError();
				}
			}
			if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				if (test(getValue(index) && index < size && goodArray)) {
					T el = pArr[index];
					return test(reply(__cmdGet) && putValue(el));
				} else {
					// reply(0xB3); // Debug point
					return replyError();
				}
			}
			// reply(0xB4); // Debug point
			return replyError();
		}




		//-----------------------------------------------------------------------
		// process channel set set array
		//-----------------------------------------------------------------------

		/** Member function that processes set array requests on a specific channel.
		@see SetArrayFn
		*/
		template <typename T>
		struct ChannelSetArrayFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, T*& __pArr, size_t& __maxSize, size_t __finalSize);

		};

		/** Process a set array command on a specific channel. Calls a ChannelSetArrayFn. */
		template <typename T>
		bool processChannelSetArray(prot_cmd_t __cmdSet, typename ChannelSetArrayFn<T>::type __arrFn) {
			prot_chan_t chan;
			prot_size_t maxSize;
			T* pArr;
			maxSize = 0;
			bool goodArray = test(getValue<prot_chan_t>(chan) && target_ && (target_->*__arrFn)(chan, pArr, maxSize, 0));

			int subCmd;
			if (!getValue<int>(subCmd)) {
				return replyError();
			}
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				if (goodArray) {
					return test(reply(__cmdSet) && putValue(maxSize));
				} else {
					return replyError();
				}
			} else if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				T el;
				if (test(getValue(index) && getValue(el) && index < maxSize && goodArray)) {
					if (index == 0) {
						if (!test(target_ && (target_->*__arrFn)(chan, pArr, maxSize, 0))) {
							return replyError();
						}
					}
					pArr[index] = el;
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			} else if (subCmd == SUBCMD_ARRAY_FINISHED) {
				prot_size_t finalSize;
				if (getValue(finalSize)) {
					if (goodArray) {
						if (!test(target_ && (target_->*__arrFn)(chan, pArr, maxSize, finalSize))) {
							return replyError();
						}
					}
					return reply(__cmdSet);
				} else {
					return replyError();
				}
			}
			return replyError();
		}


		//-----------------------------------------------------------------------
		// process channel get array
		//-----------------------------------------------------------------------

		/** Member function that processes get array requests on a specific channel.
		@see GetArrayFn
		*/
		template <typename T>
		struct ChannelGetArrayFn {
			typedef bool (DEV::*type)(prot_chan_t __chan, bool __beforeGetFlag, T*& __pArr, prot_size_t& __size);
		};

		/** Process a get array command on a specific channel. Calls a ChannelGetArrayFn. */
		template <typename T>
		bool processChannelGetArray(prot_cmd_t __cmdGet, typename ChannelGetArrayFn<T>::type __arrFn) {
			prot_chan_t chan;
			prot_cmd_t subCmd;
			if (!test(getValue<prot_chan_t>(chan) && getValue(subCmd))) {
				// reply(0xB1); // Debug point
				return replyError();
			}
			prot_size_t size;
			T* pArr;
			if (subCmd == SUBCMD_ARRAY_STARTING) {
				// tell the GetArrayFn we are starting. Allows beforeGet processing
				if (!test(target_ && (target_->*__arrFn)(chan, true, pArr, size))) {
					return replyError();
				}
				return reply(__cmdGet);
			}
			// we are in the middle of getting. Call GetArrayFn with beforeGet = false
			bool goodArray = target_ && (target_->*__arrFn)(chan, false, pArr, size);
			if (subCmd == SUBCMD_ARRAY_SIZE) {
				if (goodArray) {
					return test(reply(__cmdGet) && putValue(size));
				} else {
					// reply(0xB2); // Debug point
					return replyError();
				}
			}
			if (subCmd == SUBCMD_ARRAY_ELEMENT) {
				prot_size_t index;
				if (test(getValue(index) && index < size && goodArray)) {
					T el = pArr[index];
					return test(reply(__cmdGet) && putValue(el));
				} else {
					// reply(0xB3); // Debug point
					return replyError();
				}
			}
			// reply(0xB4); // Debug point
			return replyError();
		}




		///@}
		/////////////////////////////////////////////////////////////////////////

	protected:
		DEV* target_; ///< Target device for processXXX commands
		S stream_; ///< implementation-defined serial streaming device
		bool started_ = false;
	};

}; // namespace hprot


