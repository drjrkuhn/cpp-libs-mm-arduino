/**
\file		DeviceError.h
\brief		Some extra MicroManager device error return codes
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: DeviceError.h 454 2016-02-02 22:10:42Z jkuhn $
$Author: jkuhn $
$Revision: 454 $
$Date: 2016-02-02 16:10:42 -0600 (Tue, 02 Feb 2016) $

\ingroup	Common
*/

#pragma once

#include "DeviceBase.h"
#include <string>
#include <functional>

/** Common errors */
#define ERR_UNKNOWN_POSITION	101
#define ERR_INITIALIZE_FAILED	102
#define ERR_WRITE_FAILED		103
#define ERR_CLOSE_FAILED		104
#define ERR_FIRMWARE_NOT_FOUND	105
#define ERR_PORT_OPEN_FAILED	106
#define ERR_COMMUNICATION		107
#define ERR_NO_PORT_SET			108
#define ERR_VERSION_MISMATCH	109

#define COMMON_ERR_MAXCODE		ERR_VERSION_MISMATCH

/** 
Initialize common error codes on a device. 

\ingroup	Common

Unfortunately, CDeviceBase::SetErrorText is protected in
DeviceBase.h, so we cannot use it directly. Instead, we can 
rely on C++11 lambda functions!

An example usage in a MM::Device constructor would look like this

\code{.cpp}
MyDevice::MyDevice() : initialized_(false) {
	InitializeDefaultErrorMessages();
	initCommonErrors("Arduino", 
		g_Min_MMVersion, 
		[this](int err, const char* txt) { 
			SetErrorText(err, txt); 
		});
	...
}
\endcode

@see [C++11 Lambda Functions](http://en.cppreference.com/w/cpp/language/lambda) 
for detailed notes on the [this] capture list semantics

@param __remoteName	name of the remote device (such as "Arduino")
@param __minFirmwareVersion	minimum compatible firmware version for the remote device
@param __setErrorText C++11 lambda function that sets an error for this device
*/
inline void initCommonErrors(const char* __remoteName, long __minFirmwareVersion, std::function<void(int, const char*)> __setErrorText) {
	using namespace std;
	__setErrorText(ERR_UNKNOWN_POSITION, "Requested position not available in this device");
	__setErrorText(ERR_INITIALIZE_FAILED, "Initialization of the device failed");
	__setErrorText(ERR_WRITE_FAILED, "Failed to write data to the device");
	__setErrorText(ERR_CLOSE_FAILED, "Failed closing the device");
	__setErrorText(ERR_FIRMWARE_NOT_FOUND, (string("Did not find the ") + __remoteName + " with the correct firmware.  Is it connected to this serial port?").c_str());
	__setErrorText(ERR_PORT_OPEN_FAILED, (string("Failed opening the ") + __remoteName + " USB device").c_str());
	__setErrorText(ERR_COMMUNICATION, (string("Problem communicating with the ") + __remoteName).c_str());
	__setErrorText(ERR_NO_PORT_SET, (string("Hub Device not found. The ") + __remoteName + " Hub device is needed to create this device").c_str());
	__setErrorText(ERR_VERSION_MISMATCH, (string("The firmware version on the ") + __remoteName + " is not compatible with this adapter. Please use firmware version >= " + to_string(__minFirmwareVersion)).c_str());
}

#define assertOK(RET)	assertResult((RET), __FILE__, __LINE__)

struct DeviceResultException : public std::exception{
	int error;
	const char* file;
	int line;

	DeviceResultException(int __error, const char* __file, int __line)
		: error(__error), file(__file), line(__line) { }

	template <class DEV> 
	std::string format(DEV* __device) {
		char text[MM::MaxStrLength];
		std::stringstream os;
		os << file << "(" << line << "): ";
		__device->GetName(text);
		os << " device " << text;
		__device->GetErrorText(error, text);
		os << " error " << error << ": " << text;
		return os.str();
	}
};

/** Throws an exception if the results of a standard MM device operation 
was not DEVICE_OK. Only works on results of type int. */
template <typename T>
inline void assertResult(T __result, const char* __file, int __line) throw(DeviceResultException) {
	static_assert(std::is_same<T, int>::value, "assertOK/assertResult only works for int results. Are you sure this is a device error code?");
	if (__result != DEVICE_OK) {
		throw DeviceResultException(__result, __file, __line);
	}
}

