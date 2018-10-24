/**
\defgroup DeviceProp	Device Properties
Simplifies property handling for Micromanager Devices
*/

/**
\ingroup	DeviceProp
\file		DeviceProp.h
\brief		Simplifies property handling for Micromanager Devices
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: DeviceProp.h 406 2016-01-19 02:06:41Z jkuhn $
$Author: jkuhn $
$Revision: 406 $
$Date: 2016-01-18 20:06:41 -0600 (Mon, 18 Jan 2016) $
*/


#pragma once

#include "MMDevice.h"
#include "DeviceBase.h"
#include "DevicePropHelpers.h"

#include <type_traits>
#include <limits>

namespace dprop {

	/////////////////////////////////////////////////////////////////////////////
	// PropInfo builder
	/////////////////////////////////////////////////////////////////////////////

	/**
	A builder-pattern structure to hold global initialization names, limits, etc
	for a given property.

	\ingroup DeviceProp

	Micromanager updates the property through the OnExecute member function.

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) to/from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	Use the build method to start building a PropInfo structure.
	The build takes a minimum of the device name and an initial value.
	The initial value may or may not be used, depending on the DevicePropBase
	sub-class.

	*/
	template <typename T>
	class PropInfo {
	public:

		/** Factory method to creating a PropInfo. 
		This templated version is neccessary to catch potential problems such as
		\code{.cpp}
			PropInfo<std::string> build("foo",0);
		\endcode
		which tries to inialize a std::string from a null pointer.
		*/
		template <typename U>
		static PropInfo build(const char* __name, const U __initialValue) {
			if (std::is_convertible<U, T>::value) {
				return PropInfo(__name, static_cast<T>(__initialValue));
			}
			assert(false);
			return PropInfo(__name, T());
		}

		/** Add min and max value limits to the PropInfo. Sets hasLimits to true. */
		PropInfo& withLimits(double __min, double __max) {
			minValue_ = __min;
			maxValue_ = __max;
			hasLimits_ = true;
			return *this;
		}

		/** Add a single allowed value to the PropInfo. */
		PropInfo& withAllowedValue(const T& __val) {
			allowedValues_.push_back(__val);
			return *this;
		}

		/** Add an array of allowed values to the PropInfo using an initializer list.
		For example:
		\code{.cpp}
		PropInfo<int>build("foo",1).withAllowedValues({1, 2, 3, 4});
		\endcode
		*/
		PropInfo& withAllowedValues(const std::initializer_list<T>& __list) {
			allowedValues_.insert(allowedValues_.end(), __list.begin(), __list.end());
			return *this;
		}

		/** Add several allowed values to the PropInfo from a vector. */
		PropInfo& withAllowedValues(const std::vector<T>& __v) {
			allowedValues_.insert(allowedValues_.end(), __v.begin(), __v.end());
			return *this;
		}

		/** Specify this is a pre-init property. */
		PropInfo& withIsPreInit() {
			isPreInit_ = true;
			return *this;
		}

		/** Check that this property is read-only upon creation.
		It is mainly used as a double-check during createProperty. */
		PropInfo& assertReadOnly() {
			assertIsReadOnly_ = true;
			return *this;
		}

		/** Get the property name. */
		const char* name() const {
			return name_;
		}

		/** Get the initial property value. */
		T initialValue() const {
			return initialValue_;
		}

		/** Has the withLimits() been added? */
		bool hasLimits() const {
			return hasLimits_;
		}

		/** minimum limit value. */
		double minValue() const {
			return minValue_;
		}

		/** maximum limit value. */
		double maxValue() const {
			return maxValue_;
		}

		/** has one of the withAllowedValues() been added? */
		bool hasAllowedValues() const {
			return !allowedValues_.empty();
		}

		/** get all of the allowed values. */
		std::vector<T> allowedValues() const {
			return allowedValues_;
		}

		/** Was this specified as a pre-init property? */
		bool isPreInit() const {
			return isPreInit_;
		}

		/** Was this specified as a read-only property? */
		bool isAssertReadOnly() const {
			return assertIsReadOnly_;
		}

	protected:
		PropInfo(const char* __name, const T& __initialValue) : name_(__name), initialValue_(__initialValue) { }

		const char* name_;
		T initialValue_;
		bool hasLimits_ = false;
		double minValue_ = 0;
		double maxValue_ = 0;
		bool isPreInit_ = false;
		bool assertIsReadOnly_ = false;
		std::vector<T> allowedValues_;
	};


	/////////////////////////////////////////////////////////////////////////////
	// createDeviceProp helpers
	/////////////////////////////////////////////////////////////////////////////

	/** For clarity in the createDeviceProp below
	\ingroup DeviceProp
	*/
	enum ReadType : bool {
		/** Flag: Property can be read and written to */
		PropReadWrite = false,
		/** Flag: Property is read-only*/
		PropReadOnly = true
	};

	/** Creates a property that calls an update action __pAct on __pDevice from the __propInfo.
	\ingroup DeviceProp

	The initial __val is gien as a parameter. */
	template <typename T, class D>
	int createDeviceProp(D* __pDevice, const PropInfo<T>& __propInfo, T __initialValue,
		MM::ActionFunctor* __pAct, bool __readOnly = PropReadWrite) {

		// double-check the read-only flag if the __propInfo was created with the assertReadOnly() flag
		if (__propInfo.isAssertReadOnly() && !__readOnly) {
			assert(__propInfo.isAssertReadOnly() == __readOnly);
			// Create an "ERROR" property if assert was turned off during compile.
			__pDevice->CreateProperty(__propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
			return DEVICE_INVALID_PROPERTY;
		}
		std::string sval = MarshalValue<T>(__initialValue);
		int ret = __pDevice->CreateProperty(__propInfo.name(), sval.c_str(), mm_property_type_of<T>(__initialValue), __readOnly, __pAct, __propInfo.isPreInit());
		if (ret != DEVICE_OK)
			return ret;
		if (__propInfo.hasLimits()) {
			ret = __pDevice->SetPropertyLimits(__propInfo.name(), __propInfo.minValue(), __propInfo.maxValue());
		}
		if (__propInfo.hasAllowedValues()) {
			std::vector<std::string> allowedStrings;
			for (T aval : __propInfo.allowedValues()) {
				allowedStrings.push_back(MarshalValue<T>(aval));
			}
			__pDevice->SetAllowedValues(__propInfo.name(), allowedStrings);
		}
		return ret;
	};

	/** Creates a property that calls an update __fn on __pDevice from the __propInfo.
	\ingroup DeviceProp

	The initial __val is gien as a parameter. */
	template <typename T, class D>
	int createDeviceProp(D* __pDevice, const PropInfo<T>& __propInfo, const T& __initialValue,
		int (D::*__fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
		bool __readOnly = PropReadWrite) {
		MM::Action<D>* pAct = nullptr;
		if (__fn != nullptr) {
			pAct = new MM::Action<D>(__pDevice, __fn);
		}
		return createDeviceProp(__pDevice, __propInfo, __initialValue, pAct, __readOnly);
	};

	/** Creates a property that calls an update __fn on __pDevice from the __propInfo.
	\ingroup DeviceProp

	Create MM device properties from PropInfo information.
	This version requires a device member function of the form
	int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct)

	The initial value is taken from __propInfo.
	*/
	template <typename T, class D>
	int createDeviceProp(D* __pDevice, const PropInfo<T>& __propInfo,
		int (D::*__fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
		bool __readOnly = PropReadWrite) {
		return createDeviceProp(__pDevice, __propInfo, __propInfo.initialValue(), __fn, __readOnly);
	}


	/////////////////////////////////////////////////////////////////////////////
	// DevicePropBase
	/////////////////////////////////////////////////////////////////////////////

	/**
	A class to hold and update a micromanager property.

	Devices should not create a DevicePropBase directly,
	but should create one of its derived members and then
	call createDeviceProp.

	\ingroup DeviceProp

	Micromanager updates the property through the OnExecute member function.

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	The DEV template parameter holds the device type
	*/

	template <typename T, class DEV>
	class DevicePropBase {
	public:
		virtual ~DevicePropBase() {}

		const T& getCachedValue() {
			return cachedValue_;
		}

		typedef int (DEV::*NotifyChangeFunction)(const char* __propName, const char* __propValue);

		void setNotifyChange(NotifyChangeFunction& __notifyChangeFunc) {
			notifyChangeFunc_ = __notifyChangeFunc;
		}

		/** Sets the Device Property, which updates the getCachedValue */
		int setProperty(const T& __val) {
			int ret;
			if ((ret = SetDeviceProp(pDevice_, name_, __val)) != DEVICE_OK) {
				return ret;
			}
			return notifyChangeH(__val);
		}

		int getProperty(T& __val) const {
			return GetDeviceProp<T, DEV>(pDevice_, name_, __val);
		};

		const char* name() const {
			return name_;
		}

	protected:
		DevicePropBase() {}

		T cachedValue_;
		DEV* pDevice_ = nullptr;
		const char* name_ = nullptr;
		NotifyChangeFunction notifyChangeFunc_ = nullptr;

		int notifyChangeH(const T& __val) {
			if (notifyChangeFunc_) {
				return (pDevice_->*notifyChangeFunc_)(name_, MarshalValue<T>(__val).c_str());
			}
			return DEVICE_OK;
		}

		/** Link the property to the __pDevice and initialize from the __propInfo. */
		int createDevicePropH(DEV* __pDevice, const PropInfo<T>& __propInfo,
			MM::ActionFunctor* __pAct, bool __readOnly, bool __useInitialValue) {
			assert(__pDevice != nullptr);
			pDevice_ = __pDevice;
			name_ = __propInfo.name();
			if (__useInitialValue) {
				cachedValue_ = __propInfo.initialValue();
			}
			return createDeviceProp(pDevice_, __propInfo, cachedValue_, __pAct, __readOnly);
		}

		/** Called by the properties update method. Subclasses must override. */
		virtual int OnExecute(MM::PropertyBase* __pProp, MM::ActionType __pAct) = 0;
	};

}; // namespace dprop