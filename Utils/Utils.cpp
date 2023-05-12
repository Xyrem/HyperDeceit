/*
*		File name:
*			Utils.cpp
*
*		Use:
*			Utilities for kernel and memory manipulation.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#include "Utils.hpp"

namespace Utils
{
	/*
	*	Get function information from SEH data.
	*	DISCLAIMER: THIS IMPLEMENTATION ONLY SUPPORTS KERNEL ADDRESSES!!!!!
	*/
	bool GetFunctionInformation( _In_ uint64_t Addr, _In_opt_ RUNTIME_FUNCTION* RuntimeDataOut, _In_opt_ UNWIND_INFO_HDR* UnwindInfoOut )
	{
		// Address is null?
		if (!Addr)
			return false;

		// Get base address of driver from address.
		uint64_t BaseAddress;
		if (!RtlPcToFileHeader( Addr, &BaseAddress ))
			return false;	// Was not found.

		PIMAGE_NT_HEADERS64 NT = NTHEADER( BaseAddress );
		PIMAGE_DATA_DIRECTORY ExceptionDirectory = &NT->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ];

		// Exception data not present?
		if (!ExceptionDirectory->VirtualAddress || !ExceptionDirectory->Size)
			return false;

		int NumOfFunctions = ExceptionDirectory->Size / sizeof( RUNTIME_FUNCTION );
		for (int i = 0; i < NumOfFunctions; i++)
		{
			RUNTIME_FUNCTION* RuntimeData = (RUNTIME_FUNCTION*)(BaseAddress + ExceptionDirectory->VirtualAddress + i * sizeof( RUNTIME_FUNCTION ));

			// Is the address inside the function bounds?
			if ((BaseAddress + RuntimeData->FunctionStart) <= Addr && (BaseAddress + RuntimeData->FunctionEnd) >= Addr)
			{
				// Found
				if (RuntimeDataOut)
					*RuntimeDataOut = *RuntimeData;

				if (UnwindInfoOut)
					*UnwindInfoOut = *(UNWIND_INFO_HDR*)(BaseAddress + RuntimeData->UnwindInfo);
			
				return true;
			}
		}

		// Not found
		return false;
	}
}