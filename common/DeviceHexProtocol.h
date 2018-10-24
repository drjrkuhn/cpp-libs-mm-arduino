/**
\defgroup	DeviceHexProtocol		Hexadecimal Protocol, Device (Host)

Implements HexProtocolBase on the Micromanager host side.
*/

/**
\ingroup	DeviceHexProtocol
\file		DeviceHexProtocol.h
\brief		Standard Device-side HexProtocol implementation
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: DeviceHexProtocol.h 661 2017-04-28 01:12:06Z jkuhn $
$Author: jkuhn $
$Revision: 661 $
$Date: 2017-04-27 20:12:06 -0500 (Thu, 27 Apr 2017) $

 */

/**
\ingroup DeviceHexProtocol

\page ExampleDeviceHexProtocol DeviceHexProtocol Example

About DeviceHexProtocol
==========================

DeviceHexProtocol implements a HexProtocolBase on the host (MicroManager)
side. It uses a pointer to your MicroManager device driver object 
(HexProtocolBase::target_, derived from CDeviceBase) for reading and writing. 
The HexProtocolBase::stream_ member is a string containing the name of the serial port
on the host.

You should add DeviceHexProtocol as one of the base classes of your
device handler. 

@see [Curiously recurring template pattern]
(https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
for an explanation of why we need to include the derived
type DEV as a template argument. Micromanager Devices make extensive use
of this pattern

Friendly Access
------------------------------------
Although your target DEV device is derived from CDeviceBase,
it does not have third-party access to CDeviceBase's
WriteToComPort, GetSerialAnswer, LogMessage, or PurgeComPort
methods directly. The old way was to declare DeviceHexProtocol
as a friend in your device to grant it access to WriteToComPort, etc.

The new way uses a clever trick that uses an internal accessor
proxy class that can get pointers to these protected methods and
give the target device access to the protected methods.

Examples
===============

The example below matches the example code in StreamHexProtocol.

Proxy accessor example code
--------------------------------
\code{.cpp}
#define LOG_DEVICE_HEX_PROTOCOL		// log serial transactions
#define FRIENDLY_DEV				// use old "friendly" protected member access

#include "MMDevice.h"
#include "DeviceBase.h"
#include "DeviceHexProtocol"

typedef std::uint16_t	remote_value_t;

const auto infoPort = PropInfo<std::string>::build(MM::g_Keyword_Port, "Undef").withIsPreInit();
const auto infoValue = PropInfo<remote_value_t>::build("Remote Value", 123).withLimits(0, 1000);

class MyDevice : public CGenericBase<MyDevice>, public DeviceHexProtocol<MyDevice> {

friend class DeviceHexProtocol<MyDevice>;

	MyDevice() {
		...
		propPort_.createLocalProp(this, infoPort);
		...
	}

	// implements remote device detection
	int testProtocol() {
		std::string answer;
		if (!dispatchGet(GET_FIRMWARE, answer)) {
			return ERR_COMMUNICATION;
		}
		if (answer != FIRMWARE_STR) {
			return ERR_FIRMWARE_NOT_FOUND;
		}
		return DEVICE_OK;
	}

	// example of starting the protocol
	int Initialize() {
		std::string port = propPort_.getCachedValue();
		startProtocol(this, port);
		int ret = testProtocol();
		if (ret != DEVICE_OK)
			return ret;
		...
		remoteValue_.createRemoteProp(this, this, infoValue, 
			CommandSet::build().withSet(SET_VALUE).withGet(GET_VALUE));

		// optional to peek at the serial transaction
		std::string lastTransaction = getLastLog()
		...
	}

protected:
	RemoteProp<remote_value_t, MyDevice> remoteValue_;
	LocalProp<std::string, CMyDevice> propPort_;
};
\endcode

Old "friendly" method example code
--------------------------------

To use the old "friendly" method, define the macro FRIENDLY_DEV before
including this file.
\code{.cpp}
#define LOG_DEVICE_HEX_PROTOCOL		// log serial transactions
#define FRIENDLY_DEV				// use old "friendly" protected member access

#include "MMDevice.h"
#include "DeviceBase.h"
#include "DeviceHexProtocol"

...

class MyDevice : public CGenericBase<MyDevice>, public DeviceHexProtocol<MyDevice> {
	...
};
\endcode

Logging
==================

Serial transactions are logged if LOG_DEVICE_HEX_PROTOCOL is \c \#defined
before \c \#include "DeviceHexProtocol"

use getLastLog() to get a string containing the atomic
results of the last get property or set property operation.

- A command sequence start with a single command character 
  and its hex equivalent. ie "A=0x41:" represents the single
  byte command 'A'

- Square brackets [] indicate sends. 

- Curly brackets {} indicate receives.

- Command sequences are separated by @ characters.

- [EOT] terminators are shown as '\\x4'.

- PROT_ERROR replies are ASCII_NAK characters and
  show as, `\\x15`

- **Command hex equivalent "=0xFF:", brackets "[]" and "{}"
  command terminators "@" and spaces are NOT actually 
  transmitted**. They are added to the log for clarity

- For debugging, your slave device may reply with any 
  non-standard error code to indicate *where* an error 
  occurred.  Any error code will do, as long as it does NOT
  match the original command sent by the host. For example,
  the slave may reply(0xB0) at one error point in a function
  and reply(0xB1) at another error point. I used similar
  techniques to debug the Arduino part of the protocol.

Logging Examples
------------------------------------
\code{.cpp}
	std::string log;
	remoteValue_.setProperty(31);
	log = getLastLog();
	// results in 
	//	M=0x4d: [1f\x4] {4d\x4}
	//	putCommand(SET_VALUE='M'='\x4d') 
	//		[putValue(31=0x1f) EOT] 
	//			{checkResult(SET_VALUE=0x4d) EOT}

	int val;
	remoteValue_.getProperty(val);
	log = getLastLog();
	// results in 
	//	O=0x4f: {4f\x4} {1f\x4}
	//	putCommand(GET_VALUE='O'='\x4f') 
	//		{checkResult(GET_VALUE=0x4f) EOT} 
	//			{getValue(31=0x1f) EOT}

	...
	// send the sequence 100, 110, 120, 130
	for (int i = 0; i < 4; i++) {
		std::stringstream os;
		os << (100 + 10 * i);
		AddToPropertySequence(remoteValue_.name(), os.str().c_str());
	}
	SendPropertySequence(remoteValue_.name());
	log = getLastLog();
	// results in
	//	M=0x4d: [1\x4] {4d\x4} {100\x4} @			get max array size 0x100=256
	//	M=0x4d: [1\x4] {4d\x4} {100\x4} @			(checked again)
	//	M=0x4d: [2\x4] [0\x4] [64\x4] {4d\x4} @		set value at index 0 0x64=100
	//	M=0x4d: [2\x4] [1\x4] [6e\x4] {4d\x4} @		set value at index 1 0x6e=110
	//	M=0x4d: [2\x4] [2\x4] [78\x4] {4d\x4} @		set value at index 2 0x78=120
	//	M=0x4d: [2\x4] [3\x4] [82\x4] {4d\x4} @		set value at index 3 0x82-130
	//	M=0x4d: [3\x4] [4\x4] {4d\x4}				set final length of 4
\endcode
*/

#pragma once

#include "MMDevice.h"
#include "DeviceError.h"
#include "DeviceBase.h"
#include "HexProtocol.h"

/** 
\ingroup	DeviceHexProtocol
\c \#define FRIENDLY_DEV before including this file to use the old 
way of Device member function access, where your device declares 
DeviceHexProtocol as a friend as in
\code{.cpp}
class MyDevice : public DeviceHexProtocol<MyDevice> {
	friend class DeviceHexProtocol<MyDevice>;
	...
} 
\endcode
*/
#ifdef FRIENDLY_DEV
#define USE_DEVICE_FRIEND_METHOD	1
#else
#define USE_DEVICE_FRIEND_METHOD	0
#endif

namespace hprot {

	////////////////////////////////////////////////////////////////
	/// \name Default port settings
	///@{

	const char* const g_SerialUndefinedPort = "Undefined";
	const char* const g_SerialDataBits = "8";
	const char* const g_SerialParity = "None";
	const char* const g_SerialStopBits = "1";
	const char* const g_SerialHandshaking = "Off";
	const char* const g_SerialAnswerTimeout = "500.0";
	const char* const g_SerialDelayBetweenCharsMs = "0";

	///@}
	////////////////////////////////////////////////////////////////

	/** Micromanager Devices just need the serial port name for writing streams.
	\ingroup DeviceHexProtocol */
	typedef std::string STREAM_T;

	/**

	Implements HexProtocolBase on the device side.

	\ingroup DeviceHexProtocol

	Your Device class **must** declare DeviceHexProtocol<MyDevice>
	as a friend to allow access to the device's callGetSerialAnswer,
	callWriteToComPort, and callLogMessage member functions.

	Future version of DeviceHexProtocol may use C++11 lambda
	functions to get around this protected access limitation.

	@tparam DEV MicroManager device implementing the protocol

	*/
	template <class DEV>
	class DeviceHexProtocol : public HexProtocolBase<DEV,STREAM_T> {
	protected:
		typedef HexProtocolBase<DEV, STREAM_T> BaseClass;

	public:

#if USE_DEVICE_FRIEND_METHOD > 0
		/** \ingroup DeviceHexProtocol
		We use accessor::callXXX functions throughout this class to access 
		CDeviceBase member functions. There are two versions of this mechanism. 
		This version calls GetSerialAnswer, etc directly on the target. In this 
		version, the accessor structure is the same name as our existing 
		DeviceHexProtocol class. 
		
		This version requires the Device to declare DeviceHexProtocol as a friend, as in
		\code{.cpp}
			class MyDevice : public CGenericBase<MyDevice>, public DeviceHexProtocol<MyDevice> {
				friend class DeviceHexProtocol<MyDevice>;
				...
		\endcode
		*/
		typedef DeviceHexProtocol accessor;

		/** call protected member function CDeviceBase::GetSerialAnswer on the __target device */
		static int callGetSerialAnswer(DEV* __target, const char* __portName, const char* __term, std::string& __ans) {
			return __target->GetSerialAnswer(__portName, __term, __ans);
		}
		/** call protected member function CDeviceBase::WriteToComPort on the __target device */
		static int callWriteToComPort(DEV* __target, const char* __portName, const unsigned char* __buf, unsigned __bufLength) {
			return __target->WriteToComPort(__portName, __buf, __bufLength);
		}
		/** call protected member function CDeviceBase::LogMessage(const char*,...) on the __target device */
		static int callLogMessage(DEV* __target, const char* __msg, bool __debugOnly = false) {
			return __target->LogMessage(__msg, __debugOnly);
		}
		/** call protected member function CDeviceBase::LogMessage(const string&, ...) on the __target device */
		static int callLogMessage(DEV* __target, const std::string& __msg, bool __debugOnly = false) {
			return __target->LogMessage(__msg, __debugOnly);
		}
		/** call protected member function CDeviceBase::PurgeComPort on the __target device */
		static int callPurgeComPort(DEV* __target, const char* __portName) {
			return __target->PurgeComPort(__portName);
		}
		static MM::Core* callGetCoreCallback(DEV* __target) {
			return __target->GetCoreCallback();
		}
		static int callLogMessageCode(DEV* __target, const int __errorCode, bool __debugOnly = false) {
			return __target->LogMessageCode(__errorCode, __debugOnly);			
		}


#else // if USE_DEVICE_FRIEND_METHOD == 0
		/** \ingroup DeviceHexProtocol
		We use accessor::callXXX functions throughout this class to access
		CDeviceBase member functions. There are two versions of this mechanism.
		This method calls GetSerialAnswer, etc through a proxy accessor class
		(defined as a struct for public member access). Because accessor is 
		derived from the DEV class, it has access to DEV's protected methods.

		Even with access to protected methods, accessor cannot grant
		those methods to a third party (such as a DEV* target pointer) directly.

		The accessor class *can* however, get pointers to its own protected
		member functions inherited from DEV! 
		
		Because the DEV* target is guaranteed to be a DEV*, it can use the member 
		function pointers given to it by accessor through
		\code{.cpp}(target->*&DEV::member)(arg1, arg2)\endcode This is perfectly valid C++.

		This version **does not** require the Device to declare DeviceHexProtocol 
		as a friend class.
		*/
		struct accessor : DEV {
			/** call protected member function CDeviceBase::GetSerialAnswer on the __target device */
			static int callGetSerialAnswer(DEV* __target, const char* __portName, const char* __term, std::string& __ans) {
				return (__target->*&DEV::GetSerialAnswer)(__portName, __term, __ans);
			}
			/** call protected member function CDeviceBase::WriteToComPort on the __target device */
			static int callWriteToComPort(DEV* __target, const char* __portName, const unsigned char* __buf, unsigned __bufLength) {
				return (__target->*&DEV::WriteToComPort)(__portName, __buf, __bufLength);
			}
			/** call protected member function CDeviceBase::LogMessage(const char*,...) on the __target device */
			static int callLogMessage(DEV* __target, const char* __msg, bool __debugOnly = false) {
				// get a pointer to the correct overloaded CDeviceBase::LogMessage function
				int (DEV::*fn)(const char*, bool) const = &DEV::LogMessage;
				return (__target->*fn)(__msg, __debugOnly);
			}
			/** call protected member function CDeviceBase::LogMessage(const string&, ...) on the __target device */
			static int callLogMessage(DEV* __target, const std::string& __msg, bool __debugOnly = false) {
				// get a pointer to the correct overloaded CDeviceBase::LogMessage function
				int (DEV::*fn)(const std::string&, bool) const = &DEV::LogMessage;
				return (__target->*fn)(__msg, __debugOnly);
			}
			/** call protected member function CDeviceBase::PurgeComPort on the __target device */
			static int callPurgeComPort(DEV* __target, const char* __portName) {
				return (__target->*&DEV::PurgeComPort)(__portName);
			}
			static MM::Core* callGetCoreCallback(DEV* __target) {
				return (__target->*&DEV::GetCoreCallback)();
			}
			static int callLogMessageCode(DEV* __target, const int __errorCode, bool __debugOnly = false) {
				return (__target->*&DEV::LogMessageCode)(__errorCode, __debugOnly);
			}
		};
#endif // #if USE_DEVICE_FRIEND_METHOD > 0

		/////////////////////////////////////////////////////////////////////////
		/// \name HexProtocolBase Implementation
		///
		///@{

		/** End communication. Resets the port name to "Undefined". */
		void endProtocol() override {
			BaseClass::target_ = nullptr;
			BaseClass::stream_ = g_SerialUndefinedPort;
			BaseClass::endProtocol();
		}

		/** Write a single byte to the serial port. */
		bool writeByte(prot_byte_t b) override {
			if (!BaseClass::hasStarted()) {
				return false;
			}
#ifdef LOG_DEVICE_HEX_PROTOCOL
			std::ostringstream os;
			os << "writeByte: " << std::hex << b;
			accessor::callLogMessage(BaseClass::target_, os.str().c_str());
			// clear the dispatch log
			if (!protoLogStream_.str().empty()) {
			// indicate end of last command with an @ symbol
				protoLogStream_ << "@ ";
			}
			protoLogStream_ << b << "=0x" << std::hex << int(b) << ": ";
#endif
			unsigned char buf = static_cast<unsigned char>(b);
			return (DEVICE_OK == accessor::callWriteToComPort(BaseClass::target_, BaseClass::stream_.c_str(), &buf, 1));
		}

		/** Write several bytes to the output. The term character should be included
			in the buffer, or you can use a single writeByte() to write
			the term character. */
		size_t writeBuffer(const char* buffer, size_t size) override {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
#ifdef LOG_DEVICE_HEX_PROTOCOL
			std::ostringstream os;
			std::string str(buffer, size);
			os << "writeBuffer: " << size << ":[" << str << "]";
			accessor::callLogMessage(BaseClass::target_, os.str().c_str());
			protoLogStream_ << "[" << str << "] ";
#endif
			if (DEVICE_OK == accessor::callWriteToComPort(BaseClass::target_, BaseClass::stream_.c_str(), 
					reinterpret_cast<const unsigned char*>(buffer), static_cast<unsigned>(size))) {
				return size;
			} else {
				return 0;
			}
		}

		/** Read a string of bytes from the input UNTIL a terminator character is received, or a
			timeout occurrs. The terminator character is NOT added to the end of the buffer. */
		size_t readBufferUntilTerminator(char* buffer, size_t size, char terminator) override {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
			std::string answer;
			char termString[2] = {terminator, '\0'};
#ifdef LOG_DEVICE_HEX_PROTOCOL
			std::ostringstream os;
			os << "readBufferUntilTerminator: ";
#endif
			// NOTE: callGetSerialAnswer returns answer string without the terminating characters
			if (DEVICE_OK != accessor::callGetSerialAnswer(BaseClass::target_, BaseClass::stream_.c_str(), termString, answer)) {
#ifdef LOG_DEVICE_HEX_PROTOCOL
				os << "{empty}";
				accessor::callLogMessage(BaseClass::target_, os.str().c_str());
				protoLogStream_ << "{empty} ";
#endif
				return 0;
			}
			size_t bytesRead = answer.copy(buffer, size);
			// NOTE: std::string::copy does not append a null character at the 
			// end of the copied content. 
			// This is the expected behavior for readBufferUntilTerminator, and
			// the caller is expected to null terminate the buffer.
			// HOWEVER, in the interest of safety, we will null terminate the 
			// string anyway if there is room.
			if (bytesRead < size) {
				buffer[bytesRead] = '\0';
			}
#ifdef LOG_DEVICE_HEX_PROTOCOL
			os << bytesRead << ":{" << answer << terminator << "}";
			accessor::callLogMessage(BaseClass::target_, os.str().c_str());
			protoLogStream_ << "{" << answer << terminator << "} ";
#endif
			return bytesRead;
		}

		/** Reads a string of arbitrary length from the input UNTIL a terminator character is
		received, or a timeout occurrs. The terminator character is NOT added to the end
		of the string, but the string is null terminated. readStringUntilTerminator
		is primarily used for getValue<prot_string_t>(). */
		size_t readStringUntilTerminator(prot_string_t& str, char terminator) {
			if (!BaseClass::hasStarted()) {
				return 0;
			}
			char termString[2] = { terminator, '\0' };
#ifdef LOG_DEVICE_HEX_PROTOCOL
			std::ostringstream os;
			os << "readBufferUntilTerminator: ";
#endif
			// NOTE: callGetSerialAnswer returns answer string without the terminating characters
			if (DEVICE_OK != accessor::callGetSerialAnswer(BaseClass::target_, BaseClass::stream_.c_str(), termString, str)) {
#ifdef LOG_DEVICE_HEX_PROTOCOL
				os << "{empty}";
				accessor::callLogMessage(BaseClass::target_, os.str().c_str());
				protoLogStream_ << "{empty} ";
#endif
				return 0;
			}
			size_t bytesRead = str.length();
#ifdef LOG_DEVICE_HEX_PROTOCOL
			os << bytesRead << ":{" << str << terminator << "}";
			accessor::callLogMessage(BaseClass::target_, os.str().c_str());
			protoLogStream_ << "{" << str << terminator << "} ";
#endif
			return bytesRead;
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Logging Methods
		///
		///@{


		/** Lock the stream and Resets logging of serial commands to a stringstream */
		void lockStream() override {
			lock_.Lock();
#ifdef LOG_DEVICE_HEX_PROTOCOL
			protoLogStream_.str("");
#endif
		}

		/** Unlocks the stream and finishes logging serial commands and write them to a string.
		A device can read this log string with getLastLog()
		*/
		void unlockStream() override {
			lock_.Unlock();
#ifdef LOG_DEVICE_HEX_PROTOCOL
			lastProtoLog_ = protoLogStream_.str();
#endif
		}

		/** Retrive a string containing commands and values
		sent and received over the last transaction. */
		std::string getLastLog() const {
#ifdef LOG_DEVICE_HEX_PROTOCOL
			return lastProtoLog_;
#else
			return std::string();
#endif
		}

		/** Clears the transaction log. */
		void clearLastLog() {
#ifdef LOG_DEVICE_HEX_PROTOCOL
			protoLogStream_.str("");
			lastProtoLog_.clear();
#endif
		}

		///@}
		/////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////////
		/// \name Serial Detection Methods
		///
		///@{

		/** Helper method to clears the serial port buffers	*/
		int purgeComPort() {
			BaseClass::StreamGuard(this);
			if (!BaseClass::hasStarted()) {
				return ERR_NO_PORT_SET;
			}
			return accessor::callPurgeComPort(BaseClass::target_, BaseClass::stream_.c_str());
		}

		/** Implementation-specific function that detects whether a slave device is present
		on the stream during a tryStream.

		In addition to determining if the slave device is alive. It is a good idea to also
		fetch and check the slave's firmware string.

		\warning endProtocol() is called after testProtocol(), so the protocol functions 
		will no longer work after a call to tryStream().

		@return DEVICE_OK if the correct device was detected on the stream.
		*/
		virtual int testProtocol() = 0;

		/** Used by DEV::DetectDevice to determine if a given serial port is actively
			connected to a valid slave device. 
		
			Pseudo-code for the detection process
			\code{.cpp}
			tryStream(DEV* __target, std::string __stream, long __baudRate) 
			{
				... // Call a series of methods that boil down to __target->setupStreamPort(__stream);
				this->startProtocol(__target, __stream);
				this->purgeComPort();
				int ret = this->testProtocol();
				this->endProtocol();
				if (ret == DEVICE_OK) {
					... // cleanup __target
					return MM::CanCommunicate;
				} else {
					... // cleanup __target
					return MM::MM::CanNotCommunicate;
				}
			}
			\endcode

			\warning endProtocol() is called after testProtocol(), so the protocol functions will no longer work.

			@param __target	pointer to device to check
			@param __stream serial port name to check, usually taken from some preInit "port" property.
			@param __baudRate baud-rate to try (must be same as Arudino Serial.begin(baudRate) setting.
			healthy slave device on this __stream.
			@return @see MM::DeviceDetectionStatus
		*/
		MM::DeviceDetectionStatus tryStream(DEV* __target, std::string __stream, long __baudRate) {
			BaseClass::StreamGuard(this);
			MM::DeviceDetectionStatus result = MM::Misconfigured;
			char defaultAnswerTimeout[MM::MaxStrLength];
			try {
				// convert stream name to lower case
				std::string streamLowerCase = __stream;
				std::transform(streamLowerCase.begin(), streamLowerCase.end(), streamLowerCase.begin(), ::tolower);
				if (0 < streamLowerCase.length() && 0 != streamLowerCase.compare("undefined") && 0 != streamLowerCase.compare("unknown")) {
					const char* streamName = __stream.c_str();
					result = MM::CanNotCommunicate;

					MM::Core* core = accessor::callGetCoreCallback(__target);

					// record the default answer time out
					core->GetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, defaultAnswerTimeout);

					// device specific default communication parameters for Arduino
					core->SetDeviceProperty(streamName, MM::g_Keyword_BaudRate, std::to_string(__baudRate).c_str());
					core->SetDeviceProperty(streamName, MM::g_Keyword_DataBits, g_SerialDataBits);
					core->SetDeviceProperty(streamName, MM::g_Keyword_Parity, g_SerialParity);
					core->SetDeviceProperty(streamName, MM::g_Keyword_StopBits, g_SerialStopBits);
					core->SetDeviceProperty(streamName, MM::g_Keyword_Handshaking, g_SerialHandshaking);
					core->SetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, g_SerialAnswerTimeout);
					core->SetDeviceProperty(streamName, MM::g_Keyword_DelayBetweenCharsMs, g_SerialDelayBetweenCharsMs);
					MM::Device* pS = core->GetDevice(__target, streamName);

					pS->Initialize();

					// The first second or so after opening the serial port, the Arduino is 
					// waiting for firmwareupgrades.  Simply sleep 2 seconds.
					CDeviceUtils::SleepMs(2000);
					startProtocol(__target, __stream);
					purgeComPort();
					// Try the detection function
					int ret = testProtocol();
					if (ret == DEVICE_OK) {
						// Device was detected!
						result = MM::CanCommunicate;
					} else {
						// Device was not detected. Keep result = MM::CanNotCommunicate
						accessor::callLogMessageCode(__target, ret, true);
					}
					endProtocol();
					pS->Shutdown();
					// always restore the AnswerTimeout to the default
					core->SetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, defaultAnswerTimeout);
				}
			} catch (...) {
				accessor::callLogMessage(__target, "Exception in DetectDevice tryStream!", false);
			}
			return result;
		}
		///@}
		/////////////////////////////////////////////////////////////////////////

	protected:
		/** Prevent simultaneous send/receive by guarding this lockStream */
		MMThreadLock lock_;

#ifdef LOG_DEVICE_HEX_PROTOCOL
		std::stringstream protoLogStream_;	///< current logging stream
		std::string lastProtoLog_;			///< string representation of last transaction
#endif

	};

}; // namespace hprot


