/*
WinMTR
Copyright (C)  2010-2019 Appnor MSP S.A. - http://www.appnor.com
Copyright (C) 2019-2023 Leetsoftwerx

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
// FILE:            WinMTRDialog.cpp
//
//
//*****************************************************************************
module;
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>

#pragma warning (disable : 4005)

export module WinMTR.Dialog;

export import :ClassDef;

module : private;

import :tracing;
import :exporter;
import :registry;
import :StateMachine;
import :display;
