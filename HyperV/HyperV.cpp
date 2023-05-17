/*
*		File name:
*			HyperV.cpp
*
*		Use:
*			To setup HyperV related stuff.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#include "HyperV.hpp"

namespace HyperDeceit::HyperV
{
	HvDCallTemplate OriginalHypercall;
	bool HyperVRunning;
	void* CachedHypercallPages;

	// Why are we still here? Just to suffer?
	void** HvcallCodeVa;
	uint32_t* HvlEnlightenments; uint32_t OriginalHvlEnlightenments;
	bool* HalpHvSleepEnlightenedCpuManager; bool OriginalHalpHvSleepEnlightenedCpuManager;
	int* HvlLongSpinCountMask; int OriginalHvlLongSpinCountMask;
	HAL_INTEL_ENLIGHTENMENT_INFORMATION* EnlightenmentInformation; HAL_INTEL_ENLIGHTENMENT_INFORMATION OriginalEnlightenmentInformation;

	/*
	*	Returns the enlightenment responsible for the command.
	*/
	EEnlightenments GetEnlightenmentFromCommand( ECommand Cmd )
	{
		switch (Cmd)
		{
			case ECommand::SlowFlushAddressSpace: return EEnlightenments::VirtualizedRemoteFlush;
			case ECommand::FastFlushAddressSpace: return EEnlightenments::VirtualizedLocalFlush;

			case ECommand::EnterSleepState:
			case ECommand::DebugDeviceAvailable:
				return EEnlightenments::VirtualizedSleepState;

			case ECommand::SwitchAddressSpace: return EEnlightenments::VirtualizedAddressSwitch;
			case ECommand::LongSpinWait: return EEnlightenments::NotifyLongSpinWait;
		}

		return EEnlightenments::Unknown;
	}

	/*
	*	Gets the KPRCB offset for "HypercallCachedPages".
	*	Currently only supports Windows 10 1709 - Windows 11 22H2
	*/
	uint32_t GetHypercallCachedPagesOffset()
	{
		ULONG BuildNumber = GetBuildNumber();
		if (BuildNumber >= WIN10_BN_1709 && BuildNumber <= WIN10_BN_1903)
			return 0x6080;
		else if (BuildNumber >= WIN10_BN_1909 && BuildNumber <= WIN10_BN_22H2)
			return 0x8380;
		else if (BuildNumber >= WIN11_BN_21H2 && BuildNumber <= WIN11_BN_22H2)
			return 0x8700;

		// Should never occur, as there is a check for this in the entry point...
		PANIC( PANIC_UNSUPPORTED_WINDOWS_VERSION, "Unsupported Version" );
	}

	/*
	*	IPI callback for setting the HypercallCachedPages pointer in the KPRCB.
	*/
	ULONG_PTR SetHypercallCachedPagesIPICallback( _In_ ULONG_PTR CachedPagePtr )
	{
		uint64_t* HypercallCachedPages = (uint64_t*)(uint64_t( KeGetPcr()->CurrentPrcb ) + GetHypercallCachedPagesOffset());
		*HypercallCachedPages = CachedPagePtr;
		return 0;
	}

	/*
	*	Resets and frees CachedHypercallPages if HyperV is not running.
	*/
	void Stop()
	{
		// If HyperV is running, DO NOT OVERWRITE.
		if (HyperVRunning)
			return;

		// Reset it back to 0.
		KeIpiGenericCall( SetHypercallCachedPagesIPICallback, 0 );
		MmFreeContiguousMemory( CachedHypercallPages );
	}

	/*
	*	Setup CachedHypercallPages if HyperV is not running.
	*/
	bool Initialize()
	{
		// Simply check if HyperV is running by checking the HV vendor.
		int Regs[ 4 ]{};
		__cpuid( Regs, 0x40000001 );
		HyperVRunning = Regs[ 0 ] == '1#vH';

		// Check if HyperV is NOT running.
		if (!HyperVRunning)
		{
			// When Hyper-V is off, HypercallCachedPages is null, and this is
			// accessed in multiple places and will cause a page fault if not initialized.
			CachedHypercallPages = MmAllocateContiguousMemory( 0x6000, PHYSICAL_ADDRESS{ .QuadPart = -1 } );
			if (!CachedHypercallPages)
				return false;

			// Zero out the newly allocated memory.
			memset( CachedHypercallPages, 0, 0x6000 );

			int64_t HypercallCachedPagesPhys = MmGetPhysicalAddress( CachedHypercallPages ).QuadPart;

			for (int i = 0; i < 2; i++)
				*(int64_t*)(uint64_t( CachedHypercallPages ) + 16 + i * 0x1000LL) = HypercallCachedPagesPhys + i * 0x1000LL;

			// Do an IPI on all cores to set HypercallCachedPages for every core’s processor block.
			KeIpiGenericCall( SetHypercallCachedPagesIPICallback, ULONG_PTR( CachedHypercallPages ) );
		}

		return true;
	}

	/*
	*	Gets the pointer to HalpHvSleepEnlightenedCpuManager bool from ntoskrnl.
	*	Required for certain callbacks.
	*/
	bool* FindHalpHvSleepEnlightenedCpuManager( _In_ uint64_t KernelBase )
	{
		if (!KernelBase)
			PANIC( PANIC_KERNELBASE_NULL, "Kernel base was null" );

		uint64_t Addr = Utils::FindPattern( KernelBase, "\x40\x38\x3D\xCC\xCC\xCC\xCC\x74\xCC\xB9\x05" );
		if (!Addr)
			return 0;

		// Resolves the relative reference to HalpHvSleepEnlightenedCpuManager.
		HalpHvSleepEnlightenedCpuManager = (bool*)(*(int*)(Addr + 3) + Addr + 7);
		return HalpHvSleepEnlightenedCpuManager;
	}

	/*
	*	Gets the pointer to the HAL_INTEL_ENLIGHTENMENT_INFORMATION stored in ntoskrnl.
	*	Required for certain callbacks.
	*/
	HAL_INTEL_ENLIGHTENMENT_INFORMATION* FindHvEnlightenmentInformation( _In_ uint64_t KernelBase )
	{
		if (!KernelBase)
			PANIC( PANIC_KERNELBASE_NULL, "Kernel base was null" );

		uint64_t Addr = Utils::FindPattern( KernelBase, "\x89\x05\xCC\xCC\xCC\xCC\xE8\xCC\xCC\xCC\xCC\xF6\xC3\x01\x74" );
		if (!Addr)
			return 0;

		// Resolves the relative reference to HvEnlightenmentInformation.
		EnlightenmentInformation = (HAL_INTEL_ENLIGHTENMENT_INFORMATION*)(*(int*)(Addr + 2) + Addr + 6);
		return EnlightenmentInformation;
	}

	/*
	*	Gets the HvcallCodeVa pointer in ntoskrnl.
	*	Required for the hypercall hook.
	*	NOTE: Alternatively I could have pattern scanned, but this is future proofed lol..
	*/
	void** GetHvcallCodeVa( _In_ uint64_t KernelBase )
	{
		if (!KernelBase)
			PANIC( PANIC_KERNELBASE_NULL, "Kernel base was null" );

		// Let the windows kernel find the exported routine HvlInvokeHypercall/HvcallInitiateHypercall.
		uint64_t HvlInvokeHypercall = RtlFindExportedRoutineByName( KernelBase, "HvlInvokeHypercall" );

		// Not found???
		if (!HvlInvokeHypercall)
		{
			DBG( "HvlInvokeHypercall not found" );
			return 0;
		}

		// Get function information from SEH data.
		RUNTIME_FUNCTION RuntimeData{};
		UNWIND_INFO_HDR UnwindInfo{};
		if (!Utils::GetFunctionInformation( HvlInvokeHypercall, &RuntimeData, &UnwindInfo ))
		{
			// Should literally never occur, but still handle it just in case....
			DBG( "Failed to find HvlInvokeHypercall SEH information" );
			return 0;
		}

		// Skip prologue.
		uint64_t PC = HvlInvokeHypercall + UnwindInfo.PrologueSize;

		// Walk till the end of the function is reached.
		while (PC < (HvlInvokeHypercall + RuntimeData.FunctionEnd))
		{
			hde64s HDE;
			hde64_disasm( (void*)PC, &HDE );

			// Failed to disassemble???
			if (HDE.flags & F_ERROR)
				PANIC( PANIC_FAILED_TO_DISASSEMBLE, "Failed to disassemble 0x%p", PC );

			// 48 8B 05 ?? ?? ?? ??		mov rax, cs:HvcallCodeVa
			uint32_t Opcode = *(uint32_t*)PC & 0xFFFFFF;
			if (Opcode == 0x058B48)
			{
				// Resolve HvcallCodeVa.
				void** Reference = (void**)(*(int*)(PC + 3) + PC + 7);

				// If Hyper-V is not running, HvcallCodeVa always points to HvcallpNoHypervisorPresent.
				// But if it is running, it will point to a page which is just a vmcall + ret with the rest of the page nop'd out.
				if (!MmIsAddressValid( *Reference ))
					continue;

				HvcallCodeVa = Reference;
				return Reference;
			}

			// Increment instruction pointer.
			PC += HDE.len;
		}

		return 0;
	}

	/*
	*	Gets the HvlEnlightenments pointer from ntoskrnl,
	*	Required to setup the enlightenments by HyperV / to trick windows.
	*/
	uint32_t* GetHvlEnlightenments( _In_ uint64_t KernelBase )
	{
		if (!KernelBase)
			PANIC( PANIC_KERNELBASE_NULL, "Kernel base was null" );

		uint64_t Addr = Utils::FindPattern( KernelBase, "\xF7\x05\xCC\xCC\xCC\xCC\x01\x00\x00\x00\x74\xCC\xE8" );
		if (!Addr)
			return 0;

		// Resolves the relative reference to HvlEnlightenments.
		HvlEnlightenments = (uint32_t*)(*(int*)(Addr + 2) + Addr + 10);
		return HvlEnlightenments;
	}
}