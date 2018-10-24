/**
\ingroup	ArduinoCommon
\file		CommonTestFirmware.ino
\brief		Arduino common test firmware
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id:  $
$Author: $
$Revision: $
$Date:  $

*/


#include <SPI.h>
#include <Usb.h>

//#define TEST_STRINGSTREAM
//#define TEST_BRIMACM
//#define TEST_USBDEBUGPRINT
//#define TEST_DEBUGPRINT

///////////////////////////////
/// Experimental

//#define TEST_ACMSERIAL
#define TEST_BRIMACMSERIAL
//#define TEST_NEWSTREAMWORKING

#include "../../common/Arduino/Common.h"
#include "../../common/Arduino/StringStream.h"
#include "../../common/Arduino/DebugPrint.h"
#include "../../common/Arduino/UsbDebugPrint.h"
#include "../../common/Arduino/BrimACM.h"

///////////////////////////////
/// Experimental
///
#if defined(TEST_ACMSERIAL) || defined(TEST_BRIMACMSERIAL)
#include "../../common/Arduino/experimental/ACMSerial.h"
#endif

#if defined(TEST_BRIMACMSERIAL)
#include "../../common/Arduino/experimental/BrimACMSerial.h"
#endif

#if defined(TEST_NEWSTREAMWORKING)
#include "../../common/Arduino/experimental/NewStreamWorking.h"
#endif