/**
\ingroup	Common
\file		AsciiCodes.h
\brief		The first 32 ASCII control characters defined
\date		2016
\author		Jeffrey R. Kuhn <drjrkuhn@gmail.com>
\copyright	The University of Texas at Austin

$Id: AsciiCodes.h 404 2016-01-19 00:42:24Z jkuhn $
$Author: jkuhn	$
$Revision: 404 $
$Date: 2016-01-18 18:42:24 -0600 (Mon, 18 Jan 2016) $

*/

#pragma once

#define ASCII_NULL      0x00			///< Null char
#define ASCII_SOH       0x01			///< Start of Heading
#define ASCII_STX       0x02			///< Start of Text
#define ASCII_ETX       0x03			///< End of Text
#define ASCII_EOT       0x04			///< End of Transmission
#define ASCII_ENQ       0x05			///< Enquiry
#define ASCII_ACK       0x06			///< Acknowledgment
#define ASCII_BEL       0x07			///< Bell
#define ASCII_BS        0x08			///< Back Space
#define ASCII_TAB       0x09			///< Horizontal Tab
#define ASCII_NL        0x0a			///< Line Feed
#define ASCII_VT        0x0b			///< Vertical Tab
#define ASCII_FF        0x0c			///< Form Feed
#define ASCII_CR        0x0d			///< Carriage Return
#define ASCII_SO        0x0e			///< Shift Out / X-On
#define ASCII_SI        0x0f			///< Shift In / X-Off
#define ASCII_DLE       0x10			///< Data Line Escape
#define ASCII_DC1       0x11			///< Device Control 1 (oft. XON)
#define ASCII_DC2       0x12			///< Device Control 2
#define ASCII_DC3       0x13			///< Device Control 3 (oft. XOFF)
#define ASCII_DC4       0x14			///< Device Control 4
#define ASCII_NAK       0x15			///< Negative Acknowledgement
#define ASCII_SYN       0x16			///< Synchronous Idle
#define ASCII_ETB       0x17			///< End of Transmit Block
#define ASCII_CAN       0x18			///< Cancel
#define ASCII_EM        0x19			///< End of Medium
#define ASCII_SUB       0x1a			///< Substitute
#define ASCII_ESC       0x1b			///< Escape
#define ASCII_FS        0x1c			///< File Separator
#define ASCII_GS        0x1d			///< Group Separator
#define ASCII_RS        0x1e			///< Record Separator
#define ASCII_US        0x1f			///< Unit Separator
#define ASCII_SPACE		0x20			///< Space Character
#define ASCII_DEL		0x7f			///< Delete Character
#define ASCII_MIN_TEXT	ASCII_SPACE		///< Start of text
#define ASCII_MAX_TEXT	(ASCII_DEL-1)	///< End of text

