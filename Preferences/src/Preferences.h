/**
\defgroup	Preferences		Preferences Storage
Adds preferences storage to MM config files
*/

/**
\ingroup	Preferences
\file		Preferences.h
\brief		Adds preferences storage to MM config files
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin
*/


#pragma once

#include "MMDevice.h"
#include "DeviceBase.h"

//#############################################################################
//### Constants
//#############################################################################

////////////////////////////////////////////////////////////////
/// \name Module-level constants
/// \ingroup Preferences
///@{
const char* g_deviceName = "Preferences";
const char* g_deviceDesc = "Parameter Storage for Config Files";
//@}


//#############################################################################
//### class CPreferences
//#############################################################################

/** \ingroup Preferences
Preferences "device" implementation.

A normal MMDevice driver has a set list of properties that cannot be expanded
external to the device driver. This Preferences device stores string properties,
but new properties can be added at runtime. This means a config file can add
a list of properties that can be used to configure a MM plugin.

Use the "System" group "Startup" preset to set preferences in a configuration
file. For example:
\code
# Configuration presets
# Group: System
# Preset: Startup
ConfigGroup,System,Startup,Preferences,Example-Property,Startup-Value
ConfigGroup,System,Startup,Preferences,Foo,123
ConfigGroup,System,Startup,Preferences,Far,Baz
\endcode

These properties can then be retrieved at runtime with a call to `getProperty()`.
New properties can be created at runtime with a call to `setProperty()`, or they
can be added to the "Startup" group with a call to `defineConfig()`.

Given the above 
\code{.java}
// Retrieve some property values from the config file
String fooValue = getProperty("Preferences", "Foo");
// -> "123"
String barValue = getProperty("Preferences", "Bar");
// -> "Baz"

// store a temporary property value
setProperty("Preferences", "Volatile-Prop", "I disappear");

// store a permanent property value that is saved if the config file is saved
defineConfig("System", "Startup", "Preferences", "Stored-Prop", "I am written to the .cfg file");
\endcode

*/
class CPreferences : public CGenericBase<CPreferences> {
public:
	CPreferences();

	/** Creates a new property if it does not alread exist. */
	int SetProperty(const char* name, const char* value) override;

	// MMDevice
	bool Busy() override { return false; }
	int Initialize() override;
	int Shutdown() override;
	void GetName(char* __name) const override {
		CDeviceUtils::CopyLimitedString(__name, g_deviceName);
	}
};