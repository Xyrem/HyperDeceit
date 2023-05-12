/*
*		File name:
*			Utils.hpp
*
*		Use:
*			Utilities for kernel and memory manipulation.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#pragma once
#include "..\Common.hpp"

namespace Utils
{
	bool GetFunctionInformation( _In_ uint64_t Addr, _In_opt_ RUNTIME_FUNCTION* RuntimeDataOut = 0, _In_opt_ UNWIND_INFO_HDR* UnwindInfoOut = 0);

	/*
	*	Search for a pattern in a memory block.
	*	The 0xCC byte is ignored while searching, use it as a wildcard.
	*/
	template <int T>
	static uint64_t FindPattern_C( _In_ uint64_t SearchStart, _In_ uint32_t SearchSize, _In_ const char( &CPattern )[ T ] )
	{
		uint8_t* Pattern = (uint8_t*)CPattern;
		for ( uint8_t* i = (uint8_t*)SearchStart; i < (uint8_t*)(SearchStart + SearchSize - T - 1); i++ )
		{
			bool Found = true;

			for ( int a = 0; a < T - 1; a++ )
			{
				if ( i[ a ] != Pattern[ a ] && Pattern[ a ] != 0xCC )
				{
					Found = false;
					break;
				}
			}

			if ( Found )
				return uint64_t(i);
		}

		return 0;
	}

	/*
	*	Searches for a pattern in a PE module.
	*	0xCC byte is ignored while searching, use it as a wildcard.
	*/
	template <int T>
	static uint64_t FindPattern( _In_ uint64_t Base, _In_ const char( &Pattern )[ T ] )
	{
		// Basic sanity checks.
		if ( !Base || PIMAGE_DOS_HEADER( Base )->e_magic != IMAGE_DOS_SIGNATURE )
			return 0;

		PIMAGE_NT_HEADERS64 NT = NTHEADER( Base );
		if ( NT->Signature != IMAGE_NT_SIGNATURE )
			return 0;

		PIMAGE_SECTION_HEADER SectionHeader = IMAGE_FIRST_SECTION( NT );

		for ( int i = 0; i < NT->FileHeader.NumberOfSections; i++, SectionHeader++ )
		{
			// Discardable sections are invalidated, so we should ignore them.
			if ( SectionHeader->Characteristics & IMAGE_SCN_MEM_DISCARDABLE )
				continue;

			uint64_t Address = FindPattern_C( Base + SectionHeader->VirtualAddress, SectionHeader->Misc.VirtualSize, Pattern );
			if ( Address )
				return Address;
		}

		return 0;
	}
}
