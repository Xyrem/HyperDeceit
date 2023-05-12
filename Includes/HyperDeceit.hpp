/*
*		File name:
*			HyperDeceit.hpp
*
*		Use:
*			Header file to provide an interface with HyperDeceit on a subproject.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#pragma once
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#ifndef _In_
#define _In_
#endif

namespace HyperDeceit
{
	namespace HyperV
	{
		enum class ECommand : uint64_t
		{
			SlowFlushAddressSpace = 2,
			FastFlushAddressSpace = 0x10002,

			EnterSleepState = 0x84,
			DebugDeviceAvailable = 0x87,

			SwitchAddressSpace = 0x10001,

			LongSpinWait = 0x10008
		};

		namespace Emulator
		{
			void FlushTB();
			void FlushTBAllCores();
			void SwitchAddressSpace( _In_ uint64_t NewCR3 );
			void NotifySpinWait();

			void EmulateOriginalHyperCall( _In_ ECommand Cmd, _In_ uint64_t Input );
		}
	}

	enum class EHvDStatus
	{
		Unknown,
		InvalidArguments,
		NotInitialized,
		FailedToFindEnlightenmentInformation,
		FailedToFindHalpHvSleepEnlightenedCpuManager,
		FailedToFindHvlEnlightenments,
		FailedToFindHvlInvokeHypercall,
		FailedToFindCallbacks,
		IncompatibleWindowsVersion,
		UnsupportedEnlightenment,
		Success
	};

	EHvDStatus HvDInitialize( _In_ uint64_t KernelBase );
	EHvDStatus InsertCallback( _In_ HyperV::ECommand Cmd, _In_ void(*Callback)(uint64_t Input, uint64_t Output, uint64_t OldCR3) );
	EHvDStatus HvDStop();

	const char* GetStatusString( EHvDStatus Status );
}