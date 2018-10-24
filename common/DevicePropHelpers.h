/**
\ingroup	DeviceProp
\file		DevicePropHelpers.h
\brief		Template helpers for handling for Micromanager Devices
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: DevicePropHelpers.h 404 2016-01-19 00:42:24Z jkuhn $
$Author: jkuhn $
$Revision: 404 $
$Date: 2016-01-18 18:42:24 -0600 (Mon, 18 Jan 2016) $
*/

/**
\ingroup	DeviceProp

\page AboutDeviceProp	About Device Property Helpers

About Device Property Helpers
==============================

These helper routines and classes greatly simplify the process of
creating and using Properties in MicroManager.

Each device creates a single PropInfo for each property it wants
to define. The PropInfo contains information like the property type,
its string name used by MicroManager, default value, limits, etc.

There are several helper templates for marshalling back and forth
between Properties in MicroManager and C++ types, such as
mm_property_type_of, SetProp, SetValue, ParseValue. The proper
template method is chosen based on std::enable_if switches on
the template type parameters (see below).

DevicePropBase is a helper class for creating and storing properties
on the device. It should *not* be use directly, but rather one of
its sub-classes should be used.

For example, I define a LocalPropBase below which holds a single cached
value that can be set or retrieved by either micromanager or the
device. You might use a LocalPropBase to hold device constants, read-only
information, etc.

Marshalling between C++ types and MicroManager properties
==========================================================

MicroManager property types
----------------------------------

MicroManager has three valid property types:

- MM::Float corresponds to a double
- MM::Integer corresponds to a long
- MM::String corresponds to a char* on Set(char* val) and a std::string on Get(std::string val)

Below are a set of template functions that expand the range of these values
- MM::Float will map to both float and double
- MM::Integer will map to both signed and unsigned integers of 8-32 bits
- MM::String will map to a std::string for both Set and Get and a const char* for Set

Template Selection
-----------------------------------

Template selection is based on std::enable_if and std::type_traits for a given TYPE.

In C++ terms, template selection relies on "Substitution Failure Is Not An Error", or SFINAE.
The SFINAE acronym appears all over the web on descriptions of template selection.

@see [SFINAE and enable_if](http://eli.thegreenplace.net/2014/sfinae-and-enable_if/)
For a good explanation of using std::enable_if and SFINAE.

Putting it all together
==========================

PropInfo, LocalProp, RemoteProp, and CommandSet work together for automatic property handling in micromanager.
The following example shows how a Micromanager device can perform automatic property initialization
and creation. Assume MyDevice implements the necessary HexProtocolBase methods, in the example,
MyDevice creates two local properties, propFoo and propFred, and one remote property, propBar;


\code{.cpp}
	const PropInfo<std::string> infoFoo = PropInfo<std::string("Foo", "A").withAllowedValues("A", "B", "C");
	const PropInfo<int> infoBar = PropInfo<int>("Bar", 0).withLimits(0, 100);
	const PropInfo<double> infoFred = PropInfo<double>("Fred", 0);
	...

	class MyDevice : public CGenericBase<MyDevice>, public HexProtocolBase {
	public:
		int Initialize() override {
			...
			const char* fooName = infoFoo.name();
			...
			propFoo.createLocalProp(this, infoFoo);
			propBar.createRemoteProp(this, this, infoBar, CommandSet::build().withSet(SET_BARVAL).withGet(GET_BARVAL));
			propFred.createLocalProp(this, infoFred);
		}
	protected:
		LocalProp propFoo;
		RemoteProp propBar;
		LocalProp propFred;
	};
\endcode



*/

#pragma once

#include "MMDevice.h"
#include "DeviceBase.h"

#include <type_traits>
#include <limits>

namespace dprop {
	/////////////////////////////////////////////////////////////////////////////
	// enable_if definitions
	/////////////////////////////////////////////////////////////////////////////

	/**
	\ingroup DeviceProp

	Simplified std::enable_if type definitions for MM::PropertyTypes
	*/

	/** type selector for signed or unsigned integer that can be safely cast as long
	\ingroup DeviceProp
	*/
	template <typename T>
	using enable_if_mm_integral_t = typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= sizeof(long), T>::type;

	/** type selector for float or double
	\ingroup DeviceProp
	*/
	template <typename T>
	using enable_if_mm_floating_t = typename std::enable_if<std::is_floating_point<T>::value, T>::type;

	/** type selector for reading or writing std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	using enable_if_mm_string_t = typename std::enable_if<std::is_base_of<std::string, T>::value, T>::type;

	/** type selector for reading null terminated strings (const char*).
	\note we cannot use these for writing as we do not know the size of char* buffer.
	\ingroup DeviceProp
	*/
	template <typename T>
	using enable_if_mm_constcharptr_t = typename std::enable_if<std::is_convertible<T, const char *>::value, T>::type;

	/////////////////////////////////////////////////////////////////////////////
	// MM::PropertyType marshalling
	/////////////////////////////////////////////////////////////////////////////

	/**
	get the corresponding MM::PropertyType of a given typename.
	\ingroup DeviceProp
	*/

	/** \ingroup DeviceProp */
	template <typename T>
	MM::PropertyType mm_property_type_of(T&) {
		return MM::Undef;
	}

	/** \ingroup DeviceProp */
	template <typename T>
	MM::PropertyType mm_property_type_of(enable_if_mm_integral_t<T>&) {
		return MM::Integer;
	}

	/** \ingroup DeviceProp */
	template <typename T>
	MM::PropertyType mm_property_type_of(enable_if_mm_floating_t<T>&) {
		return MM::Float;
	}

	/** \ingroup DeviceProp*/
	template <typename T>
	MM::PropertyType mm_property_type_of(enable_if_mm_string_t<T>&) {
		return MM::String;
	}

	/** \ingroup DeviceProp*/
	template <typename T>
	MM::PropertyType mm_property_type_of(enable_if_mm_constcharptr_t<T>&) {
		return MM::String;
	}

	/////////////////////////////////////////////////////////////////////////////
	// Set properties given a pointer to the property and a value
	/////////////////////////////////////////////////////////////////////////////

	/**
	Set a property for a given typename.

	\ingroup DeviceProp

	\note The argument type must be explicitely defined when used. ie:
	\code{.cpp}
	std::uint16_t value = 1234;
	SetProp<std::uint16_t>(pProp, value);
	\endcode
	*/

	/**
	Set a property for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetProp(MM::PropertyBase* __pProp, const enable_if_mm_integral_t<T>& __val) {
		long temp = static_cast<long>(__val);
		return __pProp->Set(temp);
	}

	/**
	Set a property for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetProp(MM::PropertyBase* __pProp, const enable_if_mm_floating_t<T>& __val) {
		double temp = static_cast<double>(__val);
		return __pProp->Set(temp);
	}

	/**
	Set a property for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetProp(MM::PropertyBase* __pProp, const enable_if_mm_string_t<T>& __val) {
		return __pProp->Set(__val.c_str());
	}

	/**
	Set a property for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetProp(MM::PropertyBase* __pProp, const enable_if_mm_constcharptr_t<T>& __val) {
		return __pProp->Set(__val);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Set value given a pointer to the property
	/////////////////////////////////////////////////////////////////////////////

	/**
	Get a property value for a given typename. I have switched the order
	to help denote the operation direction. The first argument is the
	lvalue and the second is the rvalue.

	\ingroup DeviceProp

	\note The argument type must be explicitely defined when used. ie:
	std::uint16_t value;
	SetValue<std::uint16_t>(value, pProp);
	// do something with value
	*/
	template <typename T>
	bool SetValue(enable_if_mm_integral_t<T>& __val, const MM::PropertyBase* __pProp) {
		long temp;
		bool ret = __pProp->Get(temp);
		__val = static_cast<T>(temp);
		return ret;
	}

	/**
	Get a property value for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetValue(enable_if_mm_floating_t<T>& __val, const MM::PropertyBase* __pProp) {
		double temp;
		bool ret = __pProp->Get(temp);
		__val = static_cast<T>(temp);
		return ret;
	}

	/**
	Get a property value for a given typename.
	\ingroup DeviceProp
	*/
	template <typename T>
	bool SetValue(enable_if_mm_string_t<T>& __val, const MM::PropertyBase* __pProp) {
		return __pProp->Get(__val);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Parse a value from a string
	/////////////////////////////////////////////////////////////////////////////

	/**
	Parse a value from a std::string

	\ingroup DeviceProp

	\note The argument type must be explicitely defined when used. ie:
	\code{.cpp}
	std::uint16_t value;
	ParseValue<std::uint16_t>(value,string);
	// do something with value
	\endcode
	*/
	template <typename T>
	void ParseValue(enable_if_mm_integral_t<T>& __val, const std::string& __str) {
		std::stringstream parser(__str);
		long temp = 0;
		parser >> temp;
		__val = static_cast<T>(temp);
	}

	/**
	Parse a value from a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	void ParseValue(enable_if_mm_floating_t<T>& __val, const std::string& __str) {
		std::stringstream parser(__str);
		double temp = 0;
		parser >> temp;
		__val = static_cast<T>(temp);
	}

	/**
	Parse a value from a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	void ParseValue(enable_if_mm_string_t<T>& __val, const std::string& __str) {
		__val = __str;
	}

	/////////////////////////////////////////////////////////////////////////////
	// Marshal a value to a string
	/////////////////////////////////////////////////////////////////////////////

	/**
	Marshal an integer to a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	std::string MarshalValue(const enable_if_mm_integral_t<T>& __val) {
		long temp = static_cast<long>(__val);
		std::stringstream os;
		os << temp;
		return os.str();
	}

	/**
	Marshal a float value to a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	std::string MarshalValue(const enable_if_mm_floating_t<T>& __val) {
		double temp = static_cast<double>(__val);
		std::stringstream os;
		os << temp;
		return os.str();
	}

	/**
	Marshal a string value to a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	std::string MarshalValue(const enable_if_mm_string_t<T>& __val) {
		return std::string(__val);
	}

	/**
	Marshal a char* value to a std::string
	\ingroup DeviceProp
	*/
	template <typename T>
	std::string MarshalValue(const enable_if_mm_constcharptr_t<T>& __val) {
		return std::string(__val);
	}

	/////////////////////////////////////////////////////////////////////////////
	// Set a device property from a name and a value
	/////////////////////////////////////////////////////////////////////////////

	/**
	Set a property by name for a given typename T on a given device DEV.

	\ingroup DeviceProp

	Devices can only set a property by name and a string value.
	This method marshals the property value to a string and
	calls the DEV->SetProperty(propname, stringvalue).

	*/

	template <typename T, class DEV>
	int SetDeviceProp(DEV* __pDev, const char* __propName, const T& __val) {
		std::string sval = MarshalValue<T>(__val);
		return __pDev->SetProperty(__propName, sval.c_str());
	}

	/////////////////////////////////////////////////////////////////////////////
	// Get a device property value from a property name
	/////////////////////////////////////////////////////////////////////////////

	/**
	Get a property by name for a given typename T on a given device DEV.

	\ingroup DeviceProp

	Devices can get properties by value. These methods marshal the
	type T to the proper __pDev->GetProperty(name, val) call.

	*/
	template <typename T, class DEV>
	int GetDeviceProp(DEV* __pDev, const char* __propName, enable_if_mm_integral_t<T>& __val) {
		int ret;
		long temp;
		if ((ret = __pDev->GetProperty(__propName, temp)) != DEVICE_OK) {
			return ret;
		}
		__val = static_cast<T>(temp);
		return ret;
	}

	/**
	Get a property by name for a given typename T on a given device DEV.

	\ingroup DeviceProp

	Devices can get properties by value. These methods marshal the
	type T to the proper __pDev->GetProperty(name, val) call.

	*/
	template <typename T, class DEV>
	int GetDeviceProp(DEV* __pDev, const char* __propName, enable_if_mm_floating_t<T>& __val) {
		int ret;
		double temp;
		if ((ret = __pDev->GetProperty(__propName, temp)) != DEVICE_OK) {
			return ret;
		}
		__val = static_cast<T>(temp);
		return ret;
	}

	/**
	Get a string property by name for a given typename T on a given device DEV.

	\ingroup DeviceProp

	Getting string device properties is a little unsafe in Micromanager. You can
	only get one by passing a char* buffer to the GetProperty method. This means
	the buffer must be big enough. We will use the constant MM::MaxStrLength
	to define the size of an intermediate buffer, temporarily
	create a buffer in free memory (instead of the stack), get the property,
	then copy the buffer to a string and free the buffer.

	*/
	template <typename T, class DEV>
	int GetDeviceProp(DEV* __pDev, const char* __propName, enable_if_mm_string_t<T>& __val) {
		char* resBuf = new char[MM::MaxStrLength];
		int ret = __pDev->GetProperty(__propName, resBuf);
		if (ret == DEVICE_OK) {
			__val.assign(resBuf);
		}
		delete resBuf;
		return ret;
	}

}; // namespace dprop