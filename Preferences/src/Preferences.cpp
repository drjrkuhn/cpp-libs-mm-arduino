/**
\ingroup	Preferences
\file		Preferences.cpp
\brief		Adds preferences storage to MM config files
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin
*/

#include "Preferences.h"

//#############################################################################
//### Module
//#############################################################################

///////////////////////////////////////////////////////////////////////////////
/// \name Exported Module API
/// @see ModuleInterface.h
///////////////////////////////////////////////////////////////////////////////
///@{

/** Initialize the device adapter module. */
MODULE_API void InitializeModuleData() {
	RegisterDevice(g_deviceName, MM::GenericDevice, g_deviceDesc);
}

/** Instantiate the named device. */
MODULE_API MM::Device* CreateDevice(const char* deviceName) {
	if (deviceName == 0) {
		return nullptr;
	}
	if (strcmp(deviceName, g_deviceName) == 0) {
		return new CPreferences;
	}
	return nullptr;
}

/** Destroy a device instance. */
MODULE_API void DeleteDevice(MM::Device* pDevice) {
	delete pDevice;
}

///@}


//#############################################################################
//### class CPreferences
//#############################################################################

///////////////////////////////////////////////////////////////////////////////
/// \name CPreferences members
///////////////////////////////////////////////////////////////////////////////
///@{

/** Default constructor */
CPreferences::CPreferences() {
}

/** Creates a new MM::String property if name does not exist. */
int CPreferences::SetProperty(const char* name, const char* value) {
	if (HasProperty(name)) {
		return CDeviceBase::SetProperty(name, value);
	} else {
		return CreateStringProperty(name, value, false);
	}
}

/** Creates an example property */
int CPreferences::Initialize() {
	int nRet = CreateStringProperty("Example-Property", "Example-Value", false);
	if (nRet != DEVICE_OK)
		return nRet;

	return DEVICE_OK;
}

/** Does nothing */
int CPreferences::Shutdown() {
	return DEVICE_OK;
}

///@}
