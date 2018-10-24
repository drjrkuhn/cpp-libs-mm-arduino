/**
\ingroup	ArduinoCommon
\file		StringStream.h
\brief		Defines a Stream class backed by a String object
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: StringStream.h 455 2016-02-02 23:39:07Z jkuhn $
$Author: jkuhn $
$Revision: 455 $
$Date: 2016-02-02 17:39:07 -0600 (Tue, 02 Feb 2016) $

*/

#pragma once

#ifndef __AVR__
#error "This libary is only available for the Arduino"
#endif

#include <WString.h>
#include <Stream.h>
#include <stdio.h> // for size_t
#if defined(TEST_STRINGSTREAM) || defined(__INTELLISENSE__)
#include <HardwareSerial.h>
#endif


/** 
A custom Arduino Stream backed by a String object. 
\ingroup	ArduinoCommon
*/
class StringStream : public Stream, protected String {
public:

	/////////////////////////////////////////////////////////////////////////
	/// \name constructors and destructors
	///
	///@{

	/** Default constructor. Creates an empty string. */
	StringStream() : head_(0) {}

	/** Create string buffer wtih an __initialCapacity. */
	StringStream(size_t __initialCapacity) : head_(0) {
		reserve(__initialCapacity);
	}

	/** Create a string buffer from another string. */
	StringStream(const String& __val) : 
		head_(0), String(__val) { }

	/** Create a string buffer from FLASH memory. */
	StringStream(const __FlashStringHelper* __val) : 
		head_(0), String(__val) { }

	///@}
	/////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////
	/// \name Print implementation
	///
	///@{
	size_t write(uint8_t __c) override {
		return concat(static_cast<char>(__c)) ? 1 : 0;
	}

	size_t write(const uint8_t* __buffer, size_t __size) override {
		return (concat(reinterpret_cast<const char*>(__buffer), __size)) ? __size : 0;
	}

	///@}
	/////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////
	/// \name Stream implementation
	///
	///@{
	int available() override;	// in INSTANTIATE

	int read() override {
		if (available()) {
			return static_cast<int>(buffer[head_++]);
		}
		return -1;
	}

	int peek() override {
		if (available()) {
			return static_cast<int>(buffer[head_]);
		}
		return -1;
	}

	/** Flush (clear) the output buffer. */
	void flush() override;	// in INSTANTIATE

	///@}
	/////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////
	/// \name unique StringStream methods
	///
	///@{

	/** Get a copy of the buffer as a String object. */
	virtual String str() const {
		return String(buffer + head_);
	}

	///@}
	/////////////////////////////////////////////////////////////////////////

	void debugPrint(Print& __printer); // in INSTANTIATE

protected:
	/** Make room by moving everything past the head to the beginning. */
	void shift(); // in INSTANTIATE

	/** Index of the next read or peek operation. */
	unsigned int head_;
};

/////////////////////////////////////////////////////////////////////////////
/// Instantiate one definition rules. 
/// @see ArduinoCommon for explanation.
/////////////////////////////////////////////////////////////////////////////
#if defined(INSTANTIATE_STRINGSTREAM_CPP) || defined(INSTANTIATE_ALL) || defined(__INTELLISENSE__)
int StringStream::available() {
	if (buffer) {
		// if we have less than half capacity left, get rid of the string before the head.
		if (head_ > capacity / 2) {
			shift();
		}
		if (head_ > len) {
			head_ = len;
		}
		return len - head_;
	}
	return 0;
}

void StringStream::flush() {
	// don't remove the memory with invalidate(), just clear the string
	len = 0;
	head_ = 0;
	if (buffer) {
		buffer[0] = '\0';
	}
}



/** Make room by moving everything past the head to the beginning. */
void StringStream::shift() {
	unsigned int newlen = len - head_;
	memmove(buffer, &buffer[head_], newlen);
	len = newlen;
	buffer[len] = '\0';
	head_ = 0;
}

void StringStream::debugPrint(Print& __printer) {
	int i;
	for (i = 0; i < head_; i++) {
		__printer.write('.');
	}
	for (; i < len; i++) {
		__printer.write(buffer[i]);
	}
}
#endif // INSTANTIATE_STRINGSTREAM_CPP

//////////////////////////////////////////////////////////
/// Test Code
//////////////////////////////////////////////////////////
#if defined(TEST_STRINGSTREAM) || defined(__INTELLISENSE__)
#include <Arduino.h>

StringStream iostr;

extern "C" {
	void setup() {
		Serial.begin(115200);
		delay(1000);
		iostr.print("Hello World! ");
		iostr.print(123);
		iostr.print("  The cow jumped over ");
		iostr.print(3.1415);
		iostr.print(" moons.@");
		Serial.println(iostr.str());
		Serial.println();

		while (iostr.available() > 0) {
			int i = iostr.read();
			if (i < 0) {
				Serial.print("\nERR: read returned ");
				Serial.println(i);
			}
			Serial.write(i);
			Serial.write(' ');
			iostr.debugPrint(Serial);
			Serial.println();
			if (i == '!') {
				String msg(" New Stuff at the end?");
				Serial.print("{! found. Adding \"");
				Serial.print(msg);
				Serial.println("\"}");
				iostr.print(msg);
			} else if (i == '@') {
				Serial.println("{@ found. FLUSHING}");
				iostr.flush();
				Serial.print("FLUSH RESULT: ");
				iostr.debugPrint(Serial);
				Serial.println();
			}
		}

		Serial.println();
		iostr.print("Line 1\nLine 2\nLine 3\n");
		while (iostr.available() > 0) {
			String line = iostr.readStringUntil('\n');
			Serial.print("Got Line: ");
			Serial.println(line);
		}
		Serial.println();
		iostr.print("1.23 120 150\n");
		Serial.print("Got float: ");
		Serial.println(iostr.parseFloat());
		Serial.print("Got int: ");
		Serial.println(iostr.parseInt());
		Serial.print("Got int: ");
		Serial.println(iostr.parseInt());
	}

	void loop() {
	}

};
#endif // TEST_STRINGSTREAM
