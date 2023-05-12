/*
*		File name:
*			HyperV.hpp
*
*		Use:
*			To setup HyperV related stuff.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#pragma once
#include "..\Common.hpp"
#include "..\Utils\Utils.hpp"
#include "..\Misc\HDE\HDE64.hpp"

namespace HyperDeceit::HyperV
{
	enum EEnlightenments : uint32_t
	{
		Unknown = 0x1BADD00D,
		VirtualizedAddressSwitch = 1,
		VirtualizedLocalFlush = 2,
		VirtualizedRemoteFlush = 4,
		NotifyLongSpinWait = 0x40,
		VirtualizedSleepState = 0x10000,
		Max = 0xFFFFFFFF
	};

	enum ECommand : uint64_t
	{
		SlowFlushAddressSpace = 2,
		FastFlushAddressSpace = 0x10002,

		EnterSleepState = 0x84,
		DebugDeviceAvailable = 0x87,
		
		SwitchAddressSpace = 0x10001,

		LongSpinWait = 0x10008
	};


	typedef uint64_t( *HvDCallTemplate )(_In_ HyperV::ECommand Command, _In_ uint64_t Arg1, _In_ uint64_t Arg2);
	
	extern HvDCallTemplate OriginalHypercall;
	extern bool HyperVRunning;


	extern void** HvcallCodeVa; extern void* OriginalHvcallCodeVa;
	extern uint32_t* HvlEnlightenments; extern uint32_t OriginalHvlEnlightenments;
	extern bool* HalpHvSleepEnlightenedCpuManager; extern bool OriginalHalpHvSleepEnlightenedCpuManager;
	extern HAL_INTEL_ENLIGHTENMENT_INFORMATION* EnlightenmentInformation; extern HAL_INTEL_ENLIGHTENMENT_INFORMATION OriginalEnlightenmentInformation;
	extern int* HvlLongSpinCountMask; extern int OriginalHvlLongSpinCountMask;


	bool Initialize( );
	void Stop( );

	EEnlightenments GetEnlightenmentFromCommand( ECommand Cmd );

	void** GetHvcallCodeVa( _In_ uint64_t KernelBase );
	uint32_t* GetHvlEnlightenments( _In_ uint64_t KernelBase );
	HAL_INTEL_ENLIGHTENMENT_INFORMATION* FindHvEnlightenmentInformation( _In_ uint64_t KernelBase );
	bool* FindHalpHvSleepEnlightenedCpuManager( _In_ uint64_t KernelBase );
}