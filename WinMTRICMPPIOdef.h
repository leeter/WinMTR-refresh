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
#pragma once

#include <minwindef.h>
#include <winternl.h>

//typedef struct _IO_STATUS_BLOCK {
//	union {
//		NTSTATUS Status;
//		PVOID    Pointer;
//	} DUMMYUNIONNAME;
//	ULONG_PTR Information;
//} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;
//
//typedef
//VOID
//(NTAPI* PIO_APC_ROUTINE) (
//	IN PVOID ApcContext,
//	IN PIO_STATUS_BLOCK IoStatusBlock,
//	IN ULONG Reserved
//	);
#define PIO_APC_ROUTINE_DEFINED



