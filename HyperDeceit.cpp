/*
*		File name:
*			HyperDeceit.cpp
*
*		Use:
*			Contains the actual logic for HyperDeceit.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#include "Common.hpp"
#include "Misc/DynamicArray.hpp"
#include "HyperV/HyperV.hpp"
#include "HyperV/Emulator/Emulator.hpp"

namespace HyperDeceit
{
	struct UserCallback_t
	{
		HyperV::ECommand Cmd;
		void(*Callback)(uint64_t, uint64_t, uint64_t);
	};

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

	uint64_t gKernelBase;
	bool Unloading;
	DynamicArray<UserCallback_t> UserCallbacks;

	/*
	*	Actual hook responsible for emulating and calling any available user callbacks
	*	available for the specific command.
	*/
	uint64_t HvDHypercallHook( _In_ HyperV::ECommand Command, _In_opt_ uint64_t Input, _In_opt_ uint64_t Output )
	{
		uint64_t Status = 0;
		uint64_t OldCR3 = __readcr3();

		// Check if this hypercall is required to be emulated or not...
		if ((HyperV::OriginalHvlEnlightenments & HyperV::GetEnlightenmentFromCommand( Command )) == 0)
			HyperV::Emulator::EmulateOriginalHyperCall( Command, Input );
		else if (HyperV::HyperVRunning)
			Status = HyperV::OriginalHypercall( Command, Input, Output );

		// Walk all user callbacks responsible for the command and invoke the callback.
		for (uint32_t i = 0; i < UserCallbacks.Size(); i++)
		{
			// To avoid a race condition between unloading.
			if (Unloading) return Status;

			UserCallback_t Callback = UserCallbacks[ i ];
			if (!Callback.Callback)
				continue;

			if (Callback.Cmd != Command)
				continue;

			Callback.Callback( Input, Output, OldCR3 );
		}

		return Status;
	}

	/*
	*	Inserts callback to intercept a specific hypercall, and also sets up additional stuff,
	*	like englightenments, callbacks etc...
	*/
	EHvDStatus InsertCallback( _In_ HyperV::ECommand Cmd, _In_ void(*Callback)(uint64_t Input, uint64_t Output, uint64_t OldCR3) )
	{
		if (!Callback)
			return EHvDStatus::InvalidArguments;

		if (!HyperV::EnlightenmentInformation)
			return EHvDStatus::NotInitialized;

		// Get enlightenment from command.
		HyperV::EEnlightenments Enlightenment = HyperV::GetEnlightenmentFromCommand( Cmd );
		if (Enlightenment == HyperV::EEnlightenments::Unknown)
			return EHvDStatus::UnsupportedEnlightenment;

		// Close your eyes and pretend this part of the code doesn't exist...
		if (!HyperV::HyperVRunning)
		{
			if (Enlightenment == HyperV::EEnlightenments::VirtualizedSleepState && (!HyperV::EnlightenmentInformation->EnterSleepState || !HyperV::EnlightenmentInformation->NotifyDebugDeviceAvailable))
			{
				// Search for the Hyper-V callbacks from HvlGetEnlightenmentInfo.
				uint64_t Addr = Utils::FindPattern( gKernelBase, "\x48\x8D\x05\xCC\xCC\xCC\xCC\x48\x89\x43\x38\x48\x8D\x05" );
				if (!Addr)
					return EHvDStatus::FailedToFindCallbacks;

				// By default these callbacks are null, and the initializer has been discarded.
				// So we should initialize them ourselves.
				HyperV::EnlightenmentInformation->EnterSleepState = Addr + *(int*)(Addr + 3) + 7; // HalpHvEnterSleepState
				Addr += 11;
				HyperV::EnlightenmentInformation->NotifyDebugDeviceAvailable = Addr + *(int*)(Addr + 3) + 7; // HvlNotifyDebugDeviceAvailable

				// Set HalpHvSleepEnlightenedCpuManager to true, so the callbacks above are invoked.
				*HyperV::HalpHvSleepEnlightenedCpuManager = true;
			}
		}

		if (Enlightenment == HyperV::EEnlightenments::NotifyLongSpinWait)
		{
			uint64_t Addr = Utils::FindPattern( gKernelBase, "\x85\x3D\xCC\xCC\xCC\xCC\x75\x1C\x8B\x05\xCC\xCC\xCC\xCC\xA8\x40" );
			if (!Addr)
				return EHvDStatus::FailedToFindCallbacks;

			Addr += *(int*)(Addr + 2) + 6;
			HyperV::OriginalHvlLongSpinCountMask = *(int*)Addr;
			HyperV::HvlLongSpinCountMask = (int*)Addr;
			*HyperV::HvlLongSpinCountMask = 1;
		}

		// Insert callback and add enlightenment.
		UserCallbacks.Insert( UserCallback_t{ Cmd, Callback } );
		*HyperV::HvlEnlightenments |= uint32_t( Enlightenment );

		return EHvDStatus::Success;
	}

	/*
	*	Initialize core components of HyperDeceit.
	*/
	EHvDStatus HvDInitialize( _In_ uint64_t KernelBase )
	{
		// You NEED to supply ntoskrnl.exe's base address as a parameter...
		if (!KernelBase)
			return EHvDStatus::InvalidArguments;

		// Unsupported windows version check..
		ULONG BuildNumber = GetBuildNumber();
		if (BuildNumber < WIN10_BN_1709 || BuildNumber > WIN11_BN_22H2)
			return EHvDStatus::IncompatibleWindowsVersion;

		gKernelBase = KernelBase;

		if (!HyperV::GetHvcallCodeVa( KernelBase ))
			return EHvDStatus::FailedToFindHvlInvokeHypercall;

		if (!HyperV::GetHvlEnlightenments( KernelBase ))
			return EHvDStatus::FailedToFindHvlEnlightenments;

		if (!HyperV::FindHvEnlightenmentInformation( KernelBase ))
			return EHvDStatus::FailedToFindEnlightenmentInformation;

		if (!HyperV::FindHalpHvSleepEnlightenedCpuManager( KernelBase ))
			return EHvDStatus::FailedToFindHalpHvSleepEnlightenedCpuManager;

		HyperV::Initialize();

		// Store the original stuff...
		HyperV::OriginalHypercall = decltype(HyperV::OriginalHypercall)(*HyperV::HvcallCodeVa);
		HyperV::OriginalHvlEnlightenments = *HyperV::HvlEnlightenments;
		HyperV::OriginalHalpHvSleepEnlightenedCpuManager = *HyperV::HalpHvSleepEnlightenedCpuManager;
		HyperV::OriginalEnlightenmentInformation = *HyperV::EnlightenmentInformation;

		// Swap the HvcallCodeVa pointer with our own hook. 
		*HyperV::HvcallCodeVa = HvDHypercallHook;
		Unloading = false;

		return EHvDStatus::Success;
	}

	/*
	*	Stop and restore everything...
	*/
	EHvDStatus HvDStop()
	{
		// Hyperdeceit isn't initialized...
		if (!HyperV::HalpHvSleepEnlightenedCpuManager)
			return EHvDStatus::NotInitialized;

		// Botch fix for now....
		Unloading = true;

		// Restore hv callbacks and disable indicator for virtualized cpu manager if
		// Hyper-V is not running.
		if (!HyperV::HyperVRunning)
		{
			*HyperV::HalpHvSleepEnlightenedCpuManager = HyperV::OriginalHalpHvSleepEnlightenedCpuManager;
			*HyperV::EnlightenmentInformation = HyperV::OriginalEnlightenmentInformation;
		}

		// Restore SpinCountMask.
		if (HyperV::HvlLongSpinCountMask)
			*HyperV::HvlLongSpinCountMask = HyperV::OriginalHvlLongSpinCountMask;

		// Restore enlightenments and hypercall.
		*HyperV::HvlEnlightenments = HyperV::OriginalHvlEnlightenments;
		*HyperV::HvcallCodeVa = HyperV::OriginalHypercall;

		// Free all user callbacks.
		// Also I think this will hit the fan at one point due to a race condition in the hook and unloading process.. Added a botch fix for now tho..
		UserCallbacks.Destroy();

		// Restore HyperV stuff.
		HyperV::Stop();

		return EHvDStatus::Success;
	}

	/*
	*	Returns a string of the status code.
	*/
	const char* GetStatusString( EHvDStatus Status )
	{
		// Could be written so much better but meh....

#define CASETOSTR(x) case x: return #x
		switch (Status)
		{
			CASETOSTR( EHvDStatus::Unknown );
			CASETOSTR( EHvDStatus::InvalidArguments );
			CASETOSTR( EHvDStatus::NotInitialized );
			CASETOSTR( EHvDStatus::FailedToFindEnlightenmentInformation );
			CASETOSTR( EHvDStatus::FailedToFindHalpHvSleepEnlightenedCpuManager );
			CASETOSTR( EHvDStatus::FailedToFindHvlEnlightenments );
			CASETOSTR( EHvDStatus::FailedToFindHvlInvokeHypercall );
			CASETOSTR( EHvDStatus::FailedToFindCallbacks );
			CASETOSTR( EHvDStatus::IncompatibleWindowsVersion );
			CASETOSTR( EHvDStatus::UnsupportedEnlightenment );
			CASETOSTR( EHvDStatus::Success );
		}
#undef CASETOSTR

		// Well clearly you had either of the 2 things occur,
		// One: You inputted some gibberish status to this function.
		// Two: Some new status codes were added but this function didn't handle them lol.
		PANIC( PANIC_UNSUPPORTED_STATEMENT, "Non-implemented status <%d>", Status );
	}
}
