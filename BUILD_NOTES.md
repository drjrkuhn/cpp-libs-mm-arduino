Micro-manager Device Driver Build Notes
========================================

The current version of MM and the device adapters need some
tweaking to build with VisualStudio 2013. Note that VisualStudio 2013 (aka VS2013)
is actually version 12.0 of the compiler (don't confuse with the release year, 2013). 
So don't be alarmed when you see v12 in these instructions

Basic setup
-----------

1. Follow the directions on Visual Studio project settings for device adapters
   https://micro-manager.org/wiki/Visual_Studio_project_settings_for_device_adapters

2. Click on View->Other Windows->Property Manager and add the MMCommon and 
   MMDeviceAdapter property sheets as instructed.
   
3. These instructions were for an earlier version of Visual Studio, and we need
   a few more modifications.
   

Boost library update
--------------------

1. Boost version 1-50-0 included in the 3rdpartypublic MM library folder does not 
   play well with VS2013. Download the latest boost version (1-58-0 worked for me) 
   and unzip it to \micromanager-source\3rdpartypublic\boost-versions.

2. You will also need matching pre-compiled boost libraries for the visual studio 
   platform (v12). You may need to do a google search.  Or you can build them yourself in VS2013 (see below)

3. Download and extract the precompiled binaries for 32-bit and 64-bit Visual Studio 
   version 12.0 and rename the directories to boost_1_58_0-lib-Win32 and 
   boost_1_58_0-lib-x64 or whatever version matches the boost include library you 
   downloaded above

4. Open the property tree in any configuration, right click on MMCommon, and 
   select "Properties" to edit it.

5. Go to Common Properties->User Macros. Find the two lines including the 
   "boost-version\boost_1_50_0" and change them to "xxx\boost_1_58_0" or
   wherever you installed boost

Alternatively, edit the following file under \micromanager-source directly:

  \micromanager-source\mmCoreAndDevices\buildscripts\VisualStudio\MMCommon.props

and change the following two lines
```xml
    <MM_BOOST_INCLUDEDIR>$(MM_3RDPARTYPUBLIC)\boost-versions\boost_1_55_0</MM_BOOST_INCLUDEDIR>
    <MM_BOOST_LIBDIR>$(MM_3RDPARTYPUBLIC)\boost-versions\boost_1_55_0-lib-$(Platform)</MM_BOOST_LIBDIR>
```
to
```xml
    <MM_BOOST_INCLUDEDIR>$(MM_3RDPARTYPUBLIC)\boost-versions\boost_1_58_0</MM_BOOST_INCLUDEDIR>
    <MM_BOOST_LIBDIR>$(MM_3RDPARTYPUBLIC)\boost-versions\boost_1_58_0-lib-$(Platform)</MM_BOOST_LIBDIR>
```

Building Boost 1.58 from sources
--------------------------------

1.  Find and run the `Developer Command Prompt for VS2103`. Do the following using that command prompt. 
    On my fresh install of VS2013, the shortcut was located in
    `C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\Shortcuts`

2.  Change to your boost_1_58_0 folder and run `bootstrap` at the command line. Stay in this folder

3.  Run the following build commands. These were taken directly from the MM 
    3rdpartypublic\boost-versions\build-1_55_0-VS2010.bat file. That .bat file does not work directly because
    VS2013 community edition doesn't have the setenv command

```
    REM 32-bit /MD
    b2 --with-system --with-atomic --with-thread --with-regex --with-chrono --with-date_time --with-timer --with-log --stagedir=stage_Win32 --build-dir=build_Win32 toolset=msvc-12.0 address-model=32 link=static runtime-link=shared
    
    REM 32-bit /MT
    b2 --with-system --with-atomic --with-thread --with-regex --with-chrono --with-date_time --with-timer --with-log --stagedir=stage_Win32 --build-dir=build_Win32 toolset=msvc-12.0 address-model=32 link=static runtime-link=static
    
    REM 64-bit /MD
    b2 --with-system --with-atomic --with-thread --with-regex --with-chrono --with-date_time --with-timer --with-log --stagedir=stage_x64 --build-dir=build_x64 toolset=msvc-12.0 address-model=64 link=static runtime-link=shared
    
    REM 64-bit /MT
    b2 --with-system --with-atomic --with-thread --with-regex --with-chrono --with-date_time --with-timer --with-log --stagedir=stage_x64 --build-dir=build_x64 toolset=msvc-12.0 address-model=64 link=static runtime-link=static
```

4.  Now move the built libraries to their own folder under boost-versions

```
    mkdir ..\boost_1_58_0-lib-Win32
    move stage_Win32\lib\* ..\boost_1_55_0-lib-Win32
    mkdir ..\boost_1_58_0-lib-x64
    move stage_x64\lib\* ..\boost_1_55_0-lib-x64
```

Platform Toolset Update
-----------------------

1. The Platform Toolset of each project needs to be updated from Windows7.1. 
   Go back to the solutions page, right click on a project and select "Properties". 

2. Make sure that "All Configurations" and "All Platforms" are selected at the top.

3. Go to Configuration Properties->General and change Platform Toolset to "Visual Studio 2013"

4. Make sure to update the Toolset on the following projects and recompile. The important one is MMDevice-SharedRuntime. This will be compiled as a subproject of your device driver. The toolsets need
to match during the linking process.

   - MMCore
   - MMDevice-SharedRuntime
   - MMDevice-StaticRuntime
   - MMCoreJ_wrap
   - MMCorePy_wrap
   
