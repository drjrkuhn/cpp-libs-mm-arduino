/**
\defgroup LocalProp		Device Properties, Local Properties
Simplifies handling of local properties for Micromanager Devices
*/

/**
\ingroup	LocalProp
\file		LocalProp.h
\brief		Simplifies device-local properties for Micromanager Devices
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: LocalProp.h 392 2016-01-16 00:44:59Z jkuhn $
$Author: jkuhn $
$Revision: 392 $
$Date: 2016-01-15 18:44:59 -0600 (Fri, 15 Jan 2016) $
*/

/**
\ingroup	LocalProp
\page		AboutLocalProp	About Local Properties

These are classes to hold device property values.

Most of the marshalling between MicroManager Property changes and
the stored property value are handled by LocalPropBase.
Specifically, micromanager will call the LocalPropBase::OnExecute()
method, which will in turn set the property value.
Property values are cached as a local copy.

LocalPropBase should not be used directly. Rather, one of its
sub-classes should be use. The following sub-classes define different
types of remote properties.

- **LocalProp** is used for properties which can be both read from
  or written to a local cached value copy. Reading the cached
  value is quick, since it does not need to go through the MM::Property
  methods

- **LocalReadOnlyProp** is used for properties for which the
  the property cannot by set by micromanager, only read.
  the getCachedValue may be updated by the device, 


*/

#pragma once

#include "DeviceProp.h"
#include "DeviceError.h"

namespace dprop {

	/////////////////////////////////////////////////////////////////////////////
	// LocalPropBase
	/////////////////////////////////////////////////////////////////////////////

	/**
	A base class for holding local property value.

	Micromanager updates the property through the OnExecute member function.

	\ingroup LocalProp

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	The DEV template parameter holds the device type
	*/
	template <typename T, typename DEV>
	class LocalPropBase : public DevicePropBase<T, DEV> {
	protected:
		typedef MM::Action<LocalPropBase<T, DEV>> ActionType;

		LocalPropBase() { }

		virtual ~LocalPropBase() { }

		bool readOnly_ = false;

		/*	Link the property to the __pDevice and initialize from the __propInfo.	*/
		int createLocalPropH(DEV* __pDevice, const PropInfo<T>& __propInfo, bool __readOnly = PropReadWrite, bool __useInitialValue = InitValueFromPropInfo) {
			MM::ActionFunctor* pAct = new ActionType(this, &LocalPropBase<T, DEV>::OnExecute);
			readOnly_ = __readOnly;
			return createDevicePropH(__pDevice, __propInfo, pAct, __readOnly, __useInitialValue);
		}


		/** Get the value before updating the property. Derived classes may override. */
		virtual int getLocalValueH(T& __val) {
			__val = cachedValue_;
			return DEVICE_OK;
		}

		/** Set the value after updating the property. Derived classes may override. */
		virtual int setLocalValueH(const T& __val) {
			cachedValue_ = __val;
			return DEVICE_OK;
		}

		/** Called by the properties update method */
		virtual int OnExecute(MM::PropertyBase* __pProp, MM::ActionType __pAct) override {
			int ret;
			// Use our helper functions above to do the work
			if (__pAct == MM::BeforeGet) {
				T temp;
				if ((ret = getLocalValueH(temp)) != DEVICE_OK) {
					return ret;
				}
				SetProp<T>(__pProp, temp);
			} else if (!readOnly_ && __pAct == MM::AfterSet) {
				T temp;
				SetValue<T>(temp, __pProp);
				if ((ret = setLocalValueH(temp)) != DEVICE_OK) {
					return ret;
				}
				return notifyChangeH(temp);
			}
			return DEVICE_OK;
		}
	};

	/**
	A class for holding a local read/write property value for a device.

	\ingroup LocalProp

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	The DEV template parameter holds the device type
	*/

	template <typename T, typename DEV>
	class LocalProp : public LocalPropBase<T, DEV> {
	public:
		/** A local read/write property that will be initialized from the PropInfo initialValue. */
		LocalProp() : LocalProp(false, true) {}

		/** A local read/write property that will be initialized with the given initialValue.
			This value overrides the PropInfo initialValue. */
		LocalProp(const T& __initialValue) : LocalProp(false, false), cachedValue_(__initialValue) {}

		virtual int createLocalProp(DEV* __pDevice, const PropInfo<T>& __propInfo) {
			return createLocalPropH(__pDevice, __propInfo, readOnly_, initFromPropInfo_);
		}

	protected:
		LocalProp(bool __readOnly, bool __initFromPropInfo) : readOnly_(__readOnly), initFromPropInfo_(__initFromPropInfo) {}

		bool readOnly_;
		bool initFromPropInfo_;
	};

	/**
	A class for holding a local read-only property value for a device.

	\ingroup LocalProp

	The T template parameter should contain the type of the member property.
	The type T should be able to auto-box and -unbox (auto-cast) from
	either MMInteger, MMFloat, or MMString.

	For example, if T=char, int, or long, then the property will be
	designated MM::Integer

	The DEV template parameter holds the device type
	*/
	template <typename T, typename DEV>
	class LocalReadOnlyProp : public LocalProp<T, DEV> {
	public:
		/** A local read-only property that will be initialized from the PropInfo initialValue. */
		LocalReadOnlyProp() : LocalProp(true, true) {}

		/** A local read-only property that will be initialized with the given initialValue.
		This value overrides the PropInfo initialValue. */
		LocalReadOnlyProp(const T& __initialValue) : LocalProp(true, false), cachedValue_(__initialValue) {}

		/** Set the cached value of a read-only property. If the property was not yet created through
		createLocalProp, then this value overrides the PropInfo initialValue. */
		int setCachedValue(const T& __val) {
			initFromPropInfo_ = false;
			int ret;
			if ((ret = setLocalValueH(__val)) != DEVICE_OK) {
				return ret;
			}
			return notifyChangeH(__val);
		}
	};

}; // namespace dprop