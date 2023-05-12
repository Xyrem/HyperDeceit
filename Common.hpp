/*
*		File name:
*			Common.hpp
*
*		Use:
*			Header files, macros, etc...
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#pragma once
#define _VERBOSE_ // Comment this line out to disable debug logging completely.
//#define _FILEVERBOSE_ // Uncomment this line to show file path, line number, and function name for debug logging.

#include <ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntimage.h>
#include <intrin.h>

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

// Basic macros and definitions.
#pragma region Definitions
// KEEP IN MIND THAT THIS HAS NO CHECK SO USE WITH CAUTION BEFORE BLINDLY PROVIDING SOME ADDRESS AS BASE!!
#define NTHEADER( Base ) PIMAGE_NT_HEADERS64( uint64_t( Base ) + PIMAGE_DOS_HEADER( Base )->e_lfanew )
#define _IMPORT_ extern "C" __declspec( dllimport )
#define GetBuildNumber( ) SharedUserData->NtBuildNumber
#define NFLAG(Var, Flag) ((Var & Flag) == 0)
#define PCODE(Code) (0xBAD00000 | Code)

#define WIN10_BN_1709 16299
#define WIN10_BN_1803 17134
#define WIN10_BN_1809 17763
#define WIN10_BN_1903 18362
#define WIN10_BN_1909 18363
#define WIN10_BN_20H1 19041
#define WIN10_BN_20H2 19042
#define WIN10_BN_21H1 19043
#define WIN10_BN_21H2 19044
#define WIN10_BN_22H2 19045
#define WIN11_BN_21H2 22000
#define WIN11_BN_22H2 22621
#pragma endregion

#pragma region PanicCodes
#define PANIC_BREAK 0 // Only use this if you don't want to bugcheck the system.
#define PANIC_UNSUPPORTED_WINDOWS_VERSION PCODE(1)
#define PANIC_KERNELBASE_NULL PCODE(2)
#define PANIC_UNSUPPORTED_STATEMENT PCODE(3)
#define PANIC_FAILED_TO_DISASSEMBLE PCODE(4)
#pragma endregion

#pragma region DebugLogging
#ifdef _VERBOSE_
#	ifdef _FILEVERBOSE_
#		define DBG( Fmt, ... )																							    \
			{																											    \
				DbgPrintEx( 0, 0, "[HyperDeceit:CORE <%s:%d %s>] "##Fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__ );	\
			}
#	else
#		define DBG( Fmt, ... )												    \
			{																    \
				DbgPrintEx( 0, 0, "[HyperDeceit:CORE] "##Fmt, __VA_ARGS__ );	\
			}
#	endif

#define PANIC( BugcheckCode, Fmt, ... )												            \
	{                                                                                           \
		DbgPrintEx( 0, 0, "\n[HyperDeceit:PANIC] !!! "##Fmt##" !!!\n", __VA_ARGS__ );           \
		BugcheckCode ? KeBugCheck( BugcheckCode ) : __debugbreak( );	            \
	}
#else
#	define DBG( ... )
#	define PANIC( BugcheckCode, ... ) BugcheckCode ? KeBugCheck( BugcheckCode ) : __debugbreak( );
#endif
#pragma endregion

#pragma region Imports
_IMPORT_ uint64_t RtlFindExportedRoutineByName( uint64_t, const char* );
_IMPORT_ uint64_t RtlPcToFileHeader( uint64_t, uint64_t* );
#pragma endregion

#pragma region Structures
struct UNWIND_INFO_HDR
{
    uint8_t Flags;
    uint8_t PrologueSize;
    uint8_t NumOfUnwindCodes;
    uint8_t FrRegOff;
};

struct RUNTIME_FUNCTION
{
    uint32_t FunctionStart;
    uint32_t FunctionEnd;
    uint32_t UnwindInfo;
};

// https://www.vergiliusproject.com/kernels/x64/Windows%2011/22H2%20(2022%20Update)/_HAL_INTEL_ENLIGHTENMENT_INFORMATION
struct HAL_INTEL_ENLIGHTENMENT_INFORMATION
{
    uint32_t Enlightenments;
    uint32_t HypervisorConnected;
    uint64_t EndOfInterrupt;
    uint64_t ApicWriteIcr;
    uint32_t Reserved0;
    uint32_t SpinCountMask;
    uint64_t LongSpinWait;
    uint64_t GetReferenceTime;
    uint64_t SetSystemSleepProperty;
    uint64_t EnterSleepState;
    uint64_t NotifyDebugDeviceAvailable;
    uint64_t MapDeviceInterrupt;
    uint64_t UnmapDeviceInterrupt;
    uint64_t RetargetDeviceInterrupt;
    uint64_t SetHpetConfig;
    uint64_t NotifyHpetEnabled;
    uint64_t QueryAssociatedProcessors;
    uint64_t ReadMultipleMsr;
    uint64_t WriteMultipleMsr;
    uint64_t ReadCpuid;
    uint64_t LpWritebackInvalidate;
    uint64_t GetMachineCheckContext;
    uint64_t SuspendPartition;
    uint64_t ResumePartition;
    uint64_t SetSystemMachineCheckProperty;
    uint64_t WheaErrorNotification;
    uint64_t GetProcessorIndexFromVpIndex;
    uint64_t SyntheticClusterIpi;
    uint64_t VpStartEnabled;
    uint64_t StartVirtualProcessor;
    uint64_t GetVpIndexFromApicId;
    uint64_t IumAccessPciDevice;
    uint64_t IumEfiRuntimeService;
    uint64_t SvmGetSystemCapabilities;
    uint64_t GetDeviceCapabilities;
    uint64_t SvmCreatePasidSpace;
    uint64_t SvmSetPasidAddressSpace;
    uint64_t SvmFlushPasid;
    uint64_t SvmAttachPasidSpace;
    uint64_t SvmDetachPasidSpace;
    uint64_t SvmEnablePasid;
    uint64_t SvmDisablePasid;
    uint64_t SvmAcknowledgePageRequest;
    uint64_t SvmCreatePrQueue;
    uint64_t SvmDeletePrQueue;
    uint64_t SvmClearPrqStalled;
    uint64_t SetDeviceAtsEnabled;
    uint64_t SetDeviceCapabilities;
    uint64_t HvDebuggerPowerHandler;
    uint64_t SetQpcBias;
    uint64_t GetQpcBias;
    uint64_t RegisterDeviceId;
    uint64_t UnregisterDeviceId;
    uint64_t AllocateDeviceDomain;
    uint64_t AttachDeviceDomain;
    uint64_t DetachDeviceDomain;
    uint64_t DeleteDeviceDomain;
    uint64_t MapDeviceLogicalRange;
    uint64_t UnmapDeviceLogicalRange;
    uint64_t MapDeviceSparsePages;
    uint64_t UnmapDeviceSparsePages;
    uint64_t GetDmaGuardEnabled;
    uint64_t UpdateMicrocode;
    uint64_t GetSintMessage;
    uint64_t SetRootFaultReportingReady;

    // Below are added from Win11 22H1!
    uint64_t ConfigureDeviceDomain;
    uint64_t UnblockDefaultDma;
    uint64_t FlushDeviceDomain;
    uint64_t FlushDeviceDomainVaList;

    // Below are added from Win11 22H2!
    uint64_t GetHybridPassthroughReservedRegions;
};
#pragma endregion