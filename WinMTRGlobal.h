/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2021 Leetsoftwerx

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//*****************************************************************************
// FILE:            WinMTRGlobal.h
//
//
// DESCRIPTION:
//   
//
// NOTES:
//    
//
//*****************************************************************************
#pragma once
#ifndef WINMTR_GLOBAL_H_
#define WINMTR_GLOBAL_H_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <version>
#include <algorithm>
#include <array>
#include <iostream>
#include <cwctype>
#include <memory>
#include <string>
#include <string_view>
#include <cwchar>
#include <fstream>
#include <format>
#include <sstream>
#include <vector>
#include <compare>
#include <iostream>
#include <span>
#include <cstdint>
#include <concepts>
#include <utility>
#include <ranges>
#include <mutex>
#include <coroutine>
#include <iterator>
#include <cstring>

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdtctl.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
  #include <afxcmn.h>
#endif 
#include <afxsock.h>





#define SAVED_PINGS 100
constexpr auto MaxHost = 256;
//#define MaxSequence 65536
#define MaxSequence 32767
//#define MaxSequence 5

//#define MAXPACKET 4096
//#define MINPACKET 64

//#define MaxTransit 4

 
//#define ICMP_ECHO		8
//#define ICMP_ECHOREPLY		0
//
//#define ICMP_TSTAMP		13
//#define ICMP_TSTAMPREPLY	14
//
//#define ICMP_TIME_EXCEEDED	11
//
//#define ICMP_HOST_UNREACHABLE 3
//
//#define MAX_UNKNOWN_HOSTS 10

//#define IP_HEADER_LENGTH   20

#ifdef DEBUG
#define TRACE_MSG(msg)										\
	{														\
	std::wostringstream dbg_msg(std::wostringstream::out);	\
	dbg_msg << msg << std::endl;							\
	OutputDebugStringW(dbg_msg.str().c_str());				\
	}
#else
#define TRACE_MSG(msg)
#endif

#endif // ifndef WINMTR_GLOBAL_H_
