/*
*		File name:
*			Emulator.cpp
*
*		Use:
*			For emulating original hypercalls.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#include "Emulator.hpp"

// Maybe fix + refactor this later.. Though submit a PR if you want to do this lol :)

namespace HyperDeceit::HyperV::Emulator
{
	/*
	*	Flushes the cache responsible for the current core.
	*/
	void FlushTB( )
	{
		uint64_t CR4 = __readcr4( );

		// CR4.PGE + CR4.PCIDE
		if ( CR4 & ((1 << 7) | (1 << 17)) )
		{
			// Toggle CR4.PGE
			__writecr4( CR4 ^ (1 << 7) );
			__writecr4( CR4 );
			return;
		}

		__writecr3( __readcr3( ) );
	}

	/*
	*	Flushes the cache for all cores by doing an IPI.
	*/
	void FlushTBAllCores( )
	{
		KeIpiGenericCall( PKIPI_BROADCAST_WORKER( &FlushTB ), 0 );
	}

	/*
	*	Switch to new address space and flush cache for the current core.
	*/
	void SwitchAddressSpace( _In_ uint64_t NewCR3 )
	{
		if ( HyperVRunning && (OriginalHvlEnlightenments & EEnlightenments::VirtualizedAddressSwitch))
		{
			HyperV::OriginalHypercall( ECommand::SwitchAddressSpace, NewCR3, 0 );
			return;
		}

		__writecr3( NewCR3 );
		FlushTB( );
	}

	/*
	*	Pretty much useless in this use case, but still added because why not...
	*/
	void NotifySpinWait( )
	{
		_mm_pause( );
	}

	/*
	*	Emulates the command if it can emulate the command.
	*/
	void EmulateOriginalHyperCall( _In_ ECommand Cmd, _In_ uint64_t Input )
	{
		switch ( Cmd )
		{
			case ECommand::SlowFlushAddressSpace: return FlushTBAllCores( );
			case ECommand::FastFlushAddressSpace: return FlushTB( );
			case ECommand::SwitchAddressSpace: return SwitchAddressSpace( Input );
			case ECommand::LongSpinWait: return NotifySpinWait( );
		}
	}
}