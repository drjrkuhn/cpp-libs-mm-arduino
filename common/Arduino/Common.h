/**
\defgroup	ArduinoCommon Common Headers for the Arduino

Common helper libraries for the Arduino
*/

/**
\ingroup	ArduinoCommon
\file		Common.h
\brief		Defines a Stream class backed by a String object
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: Common.h 411 2016-01-19 20:29:28Z jkuhn $
$Author: jkuhn $
$Revision: 411 $
$Date: 2016-01-19 14:29:28 -0600 (Tue, 19 Jan 2016) $

*/

/**
	\ingroup ArduinoCommon

	\page Common Arduino header files

	Arduino Projects and external files
	--------------------------------------

	The Arduino IDE/compiler does not have a way to include `.cpp` files outside of
	the Arduino project folder unless they are in the official "User Library" path.
	\par

	The IDE/Compiler can, however, \c \#include files given the relative path 
	in the \c \#include statement, such as
	
	\code{.cpp}
		#include "../../common/Arduino/StringStream.h"
	\endcode
	
	The upshot is that we can have common `.h` files for a project, but common 
	`.cpp` files require you to build an official Arduino library project and 
	install it in the Arduino IDE or the [Visual-Micro](http://www.visualmicro.com/)
	plugin to VisualStudio.

	Arduino "library" external .cpp files and the cludge that makes it work
	--------------------------------------------------------------
	C's [one-definition rule](http://en.cppreference.com/w/cpp/language/definition)
	becomes a problem when including a header library without a backing .cpp 
	file for static definitions and function.
	<br><br>

	To get around the no-external-.cpp-files problem in Arduino projects,
	each Arduino-specific header will check a global macro definition 
	`INSTANTIATE_xxx_CPP` and define statics, functions, members, etc 
	that would *normally* go in a .cpp file.
	<br><br>

	The `INSTANTIATE_xxx_CPP` macro should **only** be defined in a single 
	.ino or	.cpp file in the sketch folder. `INSTANTIATE_xxx_CPP` should be 
	defined before including a header files found in `common/Arduino`.
	<br><br>

	\par\b	`common/Arduino/libfile.h`

	\code{.cpp}
		#pragma once
		extern int foo();

		#ifdef INSTANTIATE_LIBFILE_CPP
		int foo()
		{
			return 10;
		}
		#endif
	\endcode

	There are two options for instantiating the definitions in one of these header
	files.

	### Option 1 - Instantate inside of the main .ino file

	Define the `INSTANTIATE_xxx_CPP` macro in your main MySketch.ino file before including
	the library file.
	<br><br>


	\par\b	`mysketch/mysketch.ino`

	\code{.cpp}
		#define INSTANTIATE_LIBFILE_CPP
		#include "../common/Arduino/libfile.h" 

		void setup() {
		}

		void loop() {
		}
	\endcode

	### Option 2 - Instantiate in a separate .cpp file

	Create a new `libfile-instance.cpp` file in your sketch folder and use that
	to instantiate the library file (the name does not really matter).

	\par\b	`mysketch/libfile-instance.cpp:`

	\code{.cpp}
		#define INSTANTIATE_LIBFILE_CPP
		#include "../common/Arduino/libfile.h"
	\endcode

	\par\b	`mysketch/mysketch.ino`

	\code{.cpp}
		#include "../common/Arduino/libfile.h" 

		void setup() {
		}

		void loop() {
		}
	\endcode

	<br><br>

	\note This is a horrible cludge, I know! However, the Arduino IDE normally concatenates
	several files into a big .ino file anyway before compiling.

*/

#pragma once

#include "StringStream.h"
#include "DebugPrint.h"

