/*
*		File name:
*			Emulator.hpp
*
*		Use:
*			For emulating original hypercalls.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#pragma once
#include "..\..\Common.hpp"
#include "..\HyperV.hpp"

namespace HyperDeceit::HyperV::Emulator
{
	void FlushTB( );
	void FlushTBAllCores( );
	void SwitchAddressSpace( _In_ uint64_t NewCR3 );
	void NotifySpinWait( );

	void EmulateOriginalHyperCall( _In_ ECommand Cmd, _In_ uint64_t Input );
}