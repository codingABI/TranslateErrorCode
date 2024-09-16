﻿/*+===================================================================
  File:      TranslateErrorCode.cpp

  Summary:   Translates the numeric standard error codes from 
             - Win32/HRESULT
             - NTSTATUS
             - Windows Update
             - LDAP
             - BugCheck/StopCode 
             to the corresponding text (if exists)

             Program should run on Windows 11/10/8.1/2022/2019/2016/2012R2

  License: CC0
  Copyright (c) 2024 codingABI

  Icon for the app: Modified icon "zoom" from Game icon pack by Kenney Vleugels (www.kenney.nl), https://kenney.nl/assets/game-icons, CC0

  Special callback function for the input edit control: Raymond Chen, https://devblogs.microsoft.com/oldnewthing/20190222-00/?p=101064

  Refs:
  https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref
  https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-code-reference2

  History:
  20240910, Initial version
  20240916, Store last input in registry

===================================================================+*/

#include "framework.h"
#include "TranslateErrorCode.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <map>

// Add libs (for Visual Studio)
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")

// Max chars for error code input edit control
#define MAXVALUELENTH 30

// Global variables
std::map<int, std::wstring> g_mWU;
std::map<int, std::wstring> g_mLDAP;
std::map<int, std::wstring> g_mBugCheck;
HINSTANCE g_hInst;
HBRUSH g_hbrOutputBackground = NULL;

// Function declarations
INT_PTR CALLBACK WndProcMainDialog(HWND, UINT, WPARAM, LPARAM);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LoadStringAsWstr

  Summary:  Get resource string as wstring

  Args:     HINSTANCE hInstance
              Handle to instance module
            UINT uID
              Resource ID

  Returns:  std::wstring

-----------------------------------------------------------------F-F*/
std::wstring LoadStringAsWstr(HINSTANCE hInstance, UINT uID) {
    PCWSTR pws;
    int cchStringLength = LoadStringW(hInstance, uID, reinterpret_cast<LPWSTR>(&pws), 0);
    if (cchStringLength > 0) return std::wstring(pws, cchStringLength); else return std::wstring();
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: isRunningUnderWine

  Summary:  Check if program is running under wine

  Args:

  Returns:  BOOL
              TRUE = Yes, running on wine
              FALSE = No, not running on wine or not running under window nt
-----------------------------------------------------------------F-F*/
BOOL isRunningUnderWine() {
    HMODULE hDLL = GetModuleHandle(L"ntdll.dll");
    if (hDLL == NULL) return FALSE;
    if (GetProcAddress(hDLL, "wine_get_version") == NULL) return FALSE; else return TRUE;
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: setBugCheckCodes

  Summary:  Fill map with error code definitions for stop codes
            Based on https://learn.microsoft.com/de-de/windows-hardware/drivers/debugger/bug-check-code-reference2

  Args:

  Returns:

-----------------------------------------------------------------F-F*/
void setBugCheckCodes() {
    g_mBugCheck[0x00000001] = L"APC_INDEX_MISMATCH";
    g_mBugCheck[0x00000002] = L"DEVICE_QUEUE_NOT_BUSY";
    g_mBugCheck[0x00000003] = L"INVALID_AFFINITY_SET";
    g_mBugCheck[0x00000004] = L"INVALID_DATA_ACCESS_TRAP";
    g_mBugCheck[0x00000005] = L"INVALID_PROCESS_ATTACH_ATTEMPT";
    g_mBugCheck[0x00000006] = L"INVALID_PROCESS_DETACH_ATTEMPT";
    g_mBugCheck[0x00000007] = L"INVALID_SOFTWARE_INTERRUPT";
    g_mBugCheck[0x00000008] = L"IRQL_NOT_DISPATCH_LEVEL";
    g_mBugCheck[0x00000009] = L"IRQL_NOT_GREATER_OR_EQUAL";
    g_mBugCheck[0x0000000A] = L"IRQL_NOT_LESS_OR_EQUAL";
    g_mBugCheck[0x0000000B] = L"NO_EXCEPTION_HANDLING_SUPPORT";
    g_mBugCheck[0x0000000C] = L"MAXIMUM_WAIT_OBJECTS_EXCEEDED";
    g_mBugCheck[0x0000000D] = L"MUTEX_LEVEL_NUMBER_VIOLATION";
    g_mBugCheck[0x0000000E] = L"NO_USER_MODE_CONTEXT";
    g_mBugCheck[0x0000000F] = L"SPIN_LOCK_ALREADY_OWNED";
    g_mBugCheck[0x00000010] = L"SPIN_LOCK_NOT_OWNED";
    g_mBugCheck[0x00000011] = L"THREAD_NOT_MUTEX_OWNER";
    g_mBugCheck[0x00000012] = L"TRAP_CAUSE_UNKNOWN";
    g_mBugCheck[0x00000013] = L"EMPTY_THREAD_REAPER_LIST";
    g_mBugCheck[0x00000014] = L"CREATE_DELETE_LOCK_NOT_LOCKED";
    g_mBugCheck[0x00000015] = L"LAST_CHANCE_CALLED_FROM_KMODE";
    g_mBugCheck[0x00000016] = L"CID_HANDLE_CREATION";
    g_mBugCheck[0x00000017] = L"CID_HANDLE_DELETION";
    g_mBugCheck[0x00000018] = L"REFERENCE_BY_POINTER";
    g_mBugCheck[0x00000019] = L"BAD_POOL_HEADER";
    g_mBugCheck[0x0000001A] = L"MEMORY_MANAGEMENT";
    g_mBugCheck[0x0000001B] = L"PFN_SHARE_COUNT";
    g_mBugCheck[0x0000001C] = L"PFN_REFERENCE_COUNT";
    g_mBugCheck[0x0000001D] = L"NO_SPIN_LOCK_AVAILABLE";
    g_mBugCheck[0x0000001E] = L"KMODE_EXCEPTION_NOT_HANDLED";
    g_mBugCheck[0x0000001F] = L"SHARED_RESOURCE_CONV_ERROR";
    g_mBugCheck[0x00000020] = L"KERNEL_APC_PENDING_DURING_EXIT";
    g_mBugCheck[0x00000021] = L"QUOTA_UNDERFLOW";
    g_mBugCheck[0x00000022] = L"FILE_SYSTEM";
    g_mBugCheck[0x00000023] = L"FAT_FILE_SYSTEM";
    g_mBugCheck[0x00000024] = L"NTFS_FILE_SYSTEM";
    g_mBugCheck[0x00000025] = L"NPFS_FILE_SYSTEM";
    g_mBugCheck[0x00000026] = L"CDFS_FILE_SYSTEM";
    g_mBugCheck[0x00000027] = L"RDR_FILE_SYSTEM";
    g_mBugCheck[0x00000028] = L"CORRUPT_ACCESS_TOKEN";
    g_mBugCheck[0x00000029] = L"SECURITY_SYSTEM";
    g_mBugCheck[0x0000002A] = L"INCONSISTENT_IRP";
    g_mBugCheck[0x0000002B] = L"PANIC_STACK_SWITCH";
    g_mBugCheck[0x0000002C] = L"PORT_DRIVER_INTERNAL";
    g_mBugCheck[0x0000002D] = L"SCSI_DISK_DRIVER_INTERNAL";
    g_mBugCheck[0x0000002E] = L"DATA_BUS_ERROR";
    g_mBugCheck[0x0000002F] = L"INSTRUCTION_BUS_ERROR";
    g_mBugCheck[0x00000030] = L"SET_OF_INVALID_CONTEXT";
    g_mBugCheck[0x00000031] = L"PHASE0_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000032] = L"PHASE1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000033] = L"UNEXPECTED_INITIALIZATION_CALL";
    g_mBugCheck[0x00000034] = L"CACHE_MANAGER";
    g_mBugCheck[0x00000035] = L"NO_MORE_IRP_STACK_LOCATIONS";
    g_mBugCheck[0x00000036] = L"DEVICE_REFERENCE_COUNT_NOT_ZERO";
    g_mBugCheck[0x00000037] = L"FLOPPY_INTERNAL_ERROR";
    g_mBugCheck[0x00000038] = L"SERIAL_DRIVER_INTERNAL";
    g_mBugCheck[0x00000039] = L"SYSTEM_EXIT_OWNED_MUTEX";
    g_mBugCheck[0x0000003A] = L"SYSTEM_UNWIND_PREVIOUS_USER";
    g_mBugCheck[0x0000003B] = L"SYSTEM_SERVICE_EXCEPTION";
    g_mBugCheck[0x0000003C] = L"INTERRUPT_UNWIND_ATTEMPTED";
    g_mBugCheck[0x0000003D] = L"INTERRUPT_EXCEPTION_NOT_HANDLED";
    g_mBugCheck[0x0000003E] = L"MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED";
    g_mBugCheck[0x0000003F] = L"NO_MORE_SYSTEM_PTES";
    g_mBugCheck[0x00000040] = L"TARGET_MDL_TOO_SMALL";
    g_mBugCheck[0x00000041] = L"MUST_SUCCEED_POOL_EMPTY";
    g_mBugCheck[0x00000042] = L"ATDISK_DRIVER_INTERNAL";
    g_mBugCheck[0x00000043] = L"NO_SUCH_PARTITION";
    g_mBugCheck[0x00000044] = L"MULTIPLE_IRP_COMPLETE_REQUESTS";
    g_mBugCheck[0x00000045] = L"INSUFFICIENT_SYSTEM_MAP_REGS";
    g_mBugCheck[0x00000046] = L"DEREF_UNKNOWN_LOGON_SESSION";
    g_mBugCheck[0x00000047] = L"REF_UNKNOWN_LOGON_SESSION";
    g_mBugCheck[0x00000048] = L"CANCEL_STATE_IN_COMPLETED_IRP";
    g_mBugCheck[0x00000049] = L"PAGE_FAULT_WITH_INTERRUPTS_OFF";
    g_mBugCheck[0x0000004A] = L"IRQL_GT_ZERO_AT_SYSTEM_SERVICE";
    g_mBugCheck[0x0000004B] = L"STREAMS_INTERNAL_ERROR";
    g_mBugCheck[0x0000004C] = L"FATAL_UNHANDLED_HARD_ERROR";
    g_mBugCheck[0x0000004D] = L"NO_PAGES_AVAILABLE";
    g_mBugCheck[0x0000004E] = L"PFN_LIST_CORRUPT";
    g_mBugCheck[0x0000004F] = L"NDIS_INTERNAL_ERROR";
    g_mBugCheck[0x00000050] = L"PAGE_FAULT_IN_NONPAGED_AREA";
    g_mBugCheck[0x00000051] = L"REGISTRY_ERROR";
    g_mBugCheck[0x00000052] = L"MAILSLOT_FILE_SYSTEM";
    g_mBugCheck[0x00000053] = L"NO_BOOT_DEVICE";
    g_mBugCheck[0x00000054] = L"LM_SERVER_INTERNAL_ERROR";
    g_mBugCheck[0x00000055] = L"DATA_COHERENCY_EXCEPTION";
    g_mBugCheck[0x00000056] = L"INSTRUCTION_COHERENCY_EXCEPTION";
    g_mBugCheck[0x00000057] = L"XNS_INTERNAL_ERROR";
    g_mBugCheck[0x00000058] = L"FTDISK_INTERNAL_ERROR";
    g_mBugCheck[0x00000059] = L"PINBALL_FILE_SYSTEM";
    g_mBugCheck[0x0000005A] = L"CRITICAL_SERVICE_FAILED";
    g_mBugCheck[0x0000005B] = L"SET_ENV_VAR_FAILED";
    g_mBugCheck[0x0000005C] = L"HAL_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000005D] = L"UNSUPPORTED_PROCESSOR";
    g_mBugCheck[0x0000005E] = L"OBJECT_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000005F] = L"SECURITY_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000060] = L"PROCESS_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000061] = L"HAL1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000062] = L"OBJECT1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000063] = L"SECURITY1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000064] = L"SYMBOLIC_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000065] = L"MEMORY1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000066] = L"CACHE_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000067] = L"CONFIG_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000068] = L"FILE_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000069] = L"IO1_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006A] = L"LPC_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006B] = L"PROCESS1_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006C] = L"REFMON_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006D] = L"SESSION1_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006E] = L"SESSION2_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000006F] = L"SESSION3_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000070] = L"SESSION4_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000071] = L"SESSION5_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000072] = L"ASSIGN_DRIVE_LETTERS_FAILED";
    g_mBugCheck[0x00000073] = L"CONFIG_LIST_FAILED";
    g_mBugCheck[0x00000074] = L"BAD_SYSTEM_CONFIG_INFO";
    g_mBugCheck[0x00000075] = L"CANNOT_WRITE_CONFIGURATION";
    g_mBugCheck[0x00000076] = L"PROCESS_HAS_LOCKED_PAGES";
    g_mBugCheck[0x00000077] = L"KERNEL_STACK_INPAGE_ERROR";
    g_mBugCheck[0x00000078] = L"PHASE0_EXCEPTION";
    g_mBugCheck[0x00000079] = L"MISMATCHED_HAL";
    g_mBugCheck[0x0000007A] = L"KERNEL_DATA_INPAGE_ERROR";
    g_mBugCheck[0x0000007B] = L"INACCESSIBLE_BOOT_DEVICE";
    g_mBugCheck[0x0000007C] = L"BUGCODE_NDIS_DRIVER";
    g_mBugCheck[0x0000007D] = L"INSTALL_MORE_MEMORY";
    g_mBugCheck[0x0000007E] = L"SYSTEM_THREAD_EXCEPTION_NOT_HANDLED";
    g_mBugCheck[0x0000007F] = L"UNEXPECTED_KERNEL_MODE_TRAP";
    g_mBugCheck[0x00000080] = L"NMI_HARDWARE_FAILURE";
    g_mBugCheck[0x00000081] = L"SPIN_LOCK_INIT_FAILURE";
    g_mBugCheck[0x00000082] = L"DFS_FILE_SYSTEM";
    g_mBugCheck[0x00000085] = L"SETUP_FAILURE";
    g_mBugCheck[0x0000008B] = L"MBR_CHECKSUM_MISMATCH";
    g_mBugCheck[0x0000008E] = L"KERNEL_MODE_EXCEPTION_NOT_HANDLED";
    g_mBugCheck[0x0000008F] = L"PP0_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000090] = L"PP1_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000092] = L"UP_DRIVER_ON_MP_SYSTEM";
    g_mBugCheck[0x00000093] = L"INVALID_KERNEL_HANDLE";
    g_mBugCheck[0x00000094] = L"KERNEL_STACK_LOCKED_AT_EXIT";
    g_mBugCheck[0x00000096] = L"INVALID_WORK_QUEUE_ITEM";
    g_mBugCheck[0x00000097] = L"BOUND_IMAGE_UNSUPPORTED";
    g_mBugCheck[0x00000098] = L"END_OF_NT_EVALUATION_PERIOD";
    g_mBugCheck[0x00000099] = L"INVALID_REGION_OR_SEGMENT";
    g_mBugCheck[0x0000009A] = L"SYSTEM_LICENSE_VIOLATION";
    g_mBugCheck[0x0000009B] = L"UDFS_FILE_SYSTEM";
    g_mBugCheck[0x0000009C] = L"MACHINE_CHECK_EXCEPTION";
    g_mBugCheck[0x0000009E] = L"USER_MODE_HEALTH_MONITOR";
    g_mBugCheck[0x0000009F] = L"DRIVER_POWER_STATE_FAILURE";
    g_mBugCheck[0x000000A0] = L"INTERNAL_POWER_ERROR";
    g_mBugCheck[0x000000A1] = L"PCI_BUS_DRIVER_INTERNAL";
    g_mBugCheck[0x000000A2] = L"MEMORY_IMAGE_CORRUPT";
    g_mBugCheck[0x000000A3] = L"ACPI_DRIVER_INTERNAL";
    g_mBugCheck[0x000000A4] = L"CNSS_FILE_SYSTEM_FILTER";
    g_mBugCheck[0x000000A5] = L"ACPI_BIOS_ERROR";
    g_mBugCheck[0x000000A7] = L"BAD_EXHANDLE";
    g_mBugCheck[0x000000AC] = L"HAL_MEMORY_ALLOCATION";
    g_mBugCheck[0x000000AD] = L"VIDEO_DRIVER_DEBUG_REPORT_REQUEST";
    g_mBugCheck[0x000000B1] = L"BGI_DETECTED_VIOLATION";
    g_mBugCheck[0x000000B4] = L"VIDEO_DRIVER_INIT_FAILURE";
    g_mBugCheck[0x000000B8] = L"ATTEMPTED_SWITCH_FROM_DPC";
    g_mBugCheck[0x000000B9] = L"CHIPSET_DETECTED_ERROR";
    g_mBugCheck[0x000000BA] = L"SESSION_HAS_VALID_VIEWS_ON_EXIT";
    g_mBugCheck[0x000000BB] = L"NETWORK_BOOT_INITIALIZATION_FAILED";
    g_mBugCheck[0x000000BC] = L"NETWORK_BOOT_DUPLICATE_ADDRESS";
    g_mBugCheck[0x000000BD] = L"INVALID_HIBERNATED_STATE";
    g_mBugCheck[0x000000BE] = L"ATTEMPTED_WRITE_TO_READONLY_MEMORY";
    g_mBugCheck[0x000000BF] = L"MUTEX_ALREADY_OWNED";
    g_mBugCheck[0x000000C1] = L"SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION";
    g_mBugCheck[0x000000C2] = L"BAD_POOL_CALLER";
    g_mBugCheck[0x000000C4] = L"DRIVER_VERIFIER_DETECTED_VIOLATION";
    g_mBugCheck[0x000000C5] = L"DRIVER_CORRUPTED_EXPOOL";
    g_mBugCheck[0x000000C6] = L"DRIVER_CAUGHT_MODIFYING_FREED_POOL";
    g_mBugCheck[0x000000C7] = L"TIMER_OR_DPC_INVALID";
    g_mBugCheck[0x000000C8] = L"IRQL_UNEXPECTED_VALUE";
    g_mBugCheck[0x000000C9] = L"DRIVER_VERIFIER_IOMANAGER_VIOLATION";
    g_mBugCheck[0x000000CA] = L"PNP_DETECTED_FATAL_ERROR";
    g_mBugCheck[0x000000CB] = L"DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS";
    g_mBugCheck[0x000000CC] = L"PAGE_FAULT_IN_FREED_SPECIAL_POOL";
    g_mBugCheck[0x000000CD] = L"PAGE_FAULT_BEYOND_END_OF_ALLOCATION";
    g_mBugCheck[0x000000CE] = L"DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS";
    g_mBugCheck[0x000000CF] = L"TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE";
    g_mBugCheck[0x000000D0] = L"DRIVER_CORRUPTED_MMPOOL";
    g_mBugCheck[0x000000D1] = L"DRIVER_IRQL_NOT_LESS_OR_EQUAL";
    g_mBugCheck[0x000000D2] = L"BUGCODE_ID_DRIVER";
    g_mBugCheck[0x000000D3] = L"DRIVER_PORTION_MUST_BE_NONPAGED";
    g_mBugCheck[0x000000D4] = L"SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD";
    g_mBugCheck[0x000000D5] = L"DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL";
    g_mBugCheck[0x000000D6] = L"DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION";
    g_mBugCheck[0x000000D7] = L"DRIVER_UNMAPPING_INVALID_VIEW";
    g_mBugCheck[0x000000D8] = L"DRIVER_USED_EXCESSIVE_PTES";
    g_mBugCheck[0x000000D9] = L"LOCKED_PAGES_TRACKER_CORRUPTION";
    g_mBugCheck[0x000000DA] = L"SYSTEM_PTE_MISUSE";
    g_mBugCheck[0x000000DB] = L"DRIVER_CORRUPTED_SYSPTES";
    g_mBugCheck[0x000000DC] = L"DRIVER_INVALID_STACK_ACCESS";
    g_mBugCheck[0x000000DE] = L"POOL_CORRUPTION_IN_FILE_AREA";
    g_mBugCheck[0x000000DF] = L"IchMPERSONATING_WORKER_THREAD";
    g_mBugCheck[0x000000E0] = L"Schwerwiegender Fehler C1250";
    g_mBugCheck[0x000000E1] = L"WORKER_THREAD_RETURNED_AT_BAD_IRQL";
    g_mBugCheck[0x000000E2] = L"MANUALLY_INITIATED_CRASH";
    g_mBugCheck[0x000000E3] = L"RESOURCE_NOT_OWNED";
    g_mBugCheck[0x000000E4] = L"WORKER_INVALID";
    g_mBugCheck[0x000000E6] = L"DRIVER_VERIFIER_DMA_VIOLATION";
    g_mBugCheck[0x000000E7] = L"INVALID_FLOATING_POINT_STATE";
    g_mBugCheck[0x000000E8] = L"INVALID_CANCEL_OF_FILE_OPEN";
    g_mBugCheck[0x000000E9] = L"ACTIVE_EX_WORKER_THREAD_TERMINATION";
    g_mBugCheck[0x000000EA] = L"THREAD_STUCK_IN_DEVICE_DRIVER";
    g_mBugCheck[0x000000EB] = L"DIRTY_MAPPED_PAGES_CONGESTION";
    g_mBugCheck[0x000000EC] = L"SESSION_HAS_VALID_SPECIAL_POOL_ON_EXIT";
    g_mBugCheck[0x000000ED] = L"UNMOUNTABLE_BOOT_VOLUME";
    g_mBugCheck[0x000000EF] = L"CRITICAL_PROCESS_DIED";
    g_mBugCheck[0x000000F0] = L"STORAGE_MINIPORT_ERROR";
    g_mBugCheck[0x000000F1] = L"SCSI_VERIFIER_DETECTED_VIOLATION";
    g_mBugCheck[0x000000F2] = L"HARDWARE_INTERRUPT_STORM";
    g_mBugCheck[0x000000F3] = L"DISORDERLY_SHUTDOWN";
    g_mBugCheck[0x000000F4] = L"CRITICAL_OBJECT_TERMINATION";
    g_mBugCheck[0x000000F5] = L"FLTMGR_FILE_SYSTEM";
    g_mBugCheck[0x000000F6] = L"PCI_VERIFIER_DETECTED_VIOLATION";
    g_mBugCheck[0x000000F7] = L"DRIVER_OVERRAN_STACK_BUFFER";
    g_mBugCheck[0x000000F8] = L"RAMDISK_BOOT_INITIALIZATION_FAILED";
    g_mBugCheck[0x000000F9] = L"DRIVER_RETURNED_STATUS_REPARSE_FOR_VOLUME_OPEN";
    g_mBugCheck[0x000000FA] = L"HTTP_DRIVER_CORRUPTED";
    g_mBugCheck[0x000000FC] = L"ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY";
    g_mBugCheck[0x000000FD] = L"DIRTY_NOWRITE_PAGES_CONGESTION";
    g_mBugCheck[0x000000FE] = L"BUGCODE_USB_DRIVER";
    g_mBugCheck[0x000000FF] = L"RESERVE_QUEUE_OVERFLOW";
    g_mBugCheck[0x00000100] = L"LOADER_BLOCK_MISMATCH";
    g_mBugCheck[0x00000101] = L"CLOCK_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x00000102] = L"DPC_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x00000103] = L"MUP_FILE_SYSTEM";
    g_mBugCheck[0x00000104] = L"AGP_INVALID_ACCESS";
    g_mBugCheck[0x00000105] = L"AGP_GART_CORRUPTION";
    g_mBugCheck[0x00000106] = L"AGP_ILLEGALLY_REPROGRAMMED";
    g_mBugCheck[0x00000108] = L"THIRD_PARTY_FILE_SYSTEM_FAILURE";
    g_mBugCheck[0x00000109] = L"CRITICAL_STRUCTURE_CORRUPTION";
    g_mBugCheck[0x0000010A] = L"APP_TAGGING_INITIALIZATION_FAILED";
    g_mBugCheck[0x0000010C] = L"FSRTL_EXTRA_CREATE_PARAMETER_VIOLATION";
    g_mBugCheck[0x0000010D] = L"WDF_VIOLATION";
    g_mBugCheck[0x0000010E] = L"VIDEO_MEMORY_MANAGEMENT_INTERNAL";
    g_mBugCheck[0x0000010F] = L"RESOURCE_MANAGER_EXCEPTION_NOT_HANDLED";
    g_mBugCheck[0x00000111] = L"RECURSIVE_NMI";
    g_mBugCheck[0x00000112] = L"MSRPC_STATE_VIOLATION";
    g_mBugCheck[0x00000113] = L"VIDEO_DXGKRNL_FATAL_ERROR";
    g_mBugCheck[0x00000114] = L"VIDEO_SHADOW_DRIVER_FATAL_ERROR";
    g_mBugCheck[0x00000115] = L"AGP_INTERNAL";
    g_mBugCheck[0x00000116] = L"VIDEO_TDR_FAILURE";
    g_mBugCheck[0x00000117] = L"VIDEO_TDR_TIMEOUT_DETECTED";
    g_mBugCheck[0x00000119] = L"VIDEO_SCHEDULER_INTERNAL_ERROR";
    g_mBugCheck[0x0000011A] = L"EM_INITIALIZATION_FAILURE";
    g_mBugCheck[0x0000011B] = L"DRIVER_RETURNED_HOLDING_CANCEL_LOCK";
    g_mBugCheck[0x0000011C] = L"ATTEMPTED_WRITE_TO_CM_PROTECTED_STORAGE";
    g_mBugCheck[0x0000011D] = L"EVENT_TRACING_FATAL_ERROR";
    g_mBugCheck[0x0000011E] = L"TOO_MANY_RECURSIVE_FAULTS";
    g_mBugCheck[0x0000011F] = L"INVALID_DRIVER_HANDLE";
    g_mBugCheck[0x00000120] = L"BITLOCKER_FATAL_ERROR";
    g_mBugCheck[0x00000121] = L"DRIVER_VIOLATION";
    g_mBugCheck[0x00000122] = L"WHEA_INTERNAL_ERROR";
    g_mBugCheck[0x00000123] = L"CRYPTO_SELF_TEST_FAILURE";
    g_mBugCheck[0x00000124] = L"WHEA_UNCORRECTABLE_ERROR";
    g_mBugCheck[0x00000125] = L"NMR_INVALID_STATE";
    g_mBugCheck[0x00000126] = L"NETIO_INVALID_POOL_CALLER";
    g_mBugCheck[0x00000127] = L"PAGE_NOT_ZERO";
    g_mBugCheck[0x00000128] = L"WORKER_THREAD_RETURNED_WITH_BAD_IO_PRIORITY";
    g_mBugCheck[0x00000129] = L"WORKER_THREAD_RETURNED_WITH_BAD_PAGING_IO_PRIORITY";
    g_mBugCheck[0x0000012A] = L"MUI_NO_VALID_SYSTEM_LANGUAGE";
    g_mBugCheck[0x0000012B] = L"FAULTY_HARDWARE_CORRUPTED_PAGE";
    g_mBugCheck[0x0000012C] = L"EXFAT_FILE_SYSTEM";
    g_mBugCheck[0x0000012D] = L"VOLSNAP_OVERLAPPED_TABLE_ACCESS";
    g_mBugCheck[0x0000012E] = L"INVALID_MDL_RANGE";
    g_mBugCheck[0x0000012F] = L"VHD_BOOT_INITIALIZATION_FAILED";
    g_mBugCheck[0x00000130] = L"DYNAMIC_ADD_PROCESSOR_MISMATCH";
    g_mBugCheck[0x00000131] = L"INVALID_EXTENDED_PROCESSOR_STATE";
    g_mBugCheck[0x00000132] = L"RESOURCE_OWNER_POINTER_INVALID";
    g_mBugCheck[0x00000133] = L"DPC_WATCHDOG_VIOLATION";
    g_mBugCheck[0x00000134] = L"DRIVE_EXTENDER";
    g_mBugCheck[0x00000135] = L"REGISTRY_FILTER_DRIVER_EXCEPTION";
    g_mBugCheck[0x00000136] = L"VHD_BOOT_HOST_VOLUME_NOT_ENOUGH_SPACE";
    g_mBugCheck[0x00000137] = L"WIN32K_HANDLE_MANAGER";
    g_mBugCheck[0x00000138] = L"GPIO_CONTROLLER_DRIVER_ERROR";
    g_mBugCheck[0x00000139] = L"KERNEL_SECURITY_CHECK_FAILURE";
    g_mBugCheck[0x0000013A] = L"KERNEL_MODE_HEAP_CORRUPTION";
    g_mBugCheck[0x0000013B] = L"PASSIVE_INTERRUPT_ERROR";
    g_mBugCheck[0x0000013C] = L"INVALID_IO_BOOST_STATE";
    g_mBugCheck[0x0000013D] = L"CRITICAL_INITIALIZATION_FAILURE";
    g_mBugCheck[0x00000140] = L"STORAGE_DEVICE_ABNORMALITY_DETECTED";
    g_mBugCheck[0x00000143] = L"PROCESSOR_DRIVER_INTERNAL";
    g_mBugCheck[0x00000144] = L"BUGCODE_USB3_DRIVER";
    g_mBugCheck[0x00000145] = L"SECURE_BOOT_VIOLATION";
    g_mBugCheck[0x00000147] = L"ABNORMAL_RESET_DETECTED";
    g_mBugCheck[0x00000149] = L"REFS_FILE_SYSTEM";
    g_mBugCheck[0x0000014A] = L"KERNEL_WMI_INTERNAL";
    g_mBugCheck[0x0000014B] = L"SOC_SUBSYSTEM_FAILURE";
    g_mBugCheck[0x0000014C] = L"FATAL_ABNORMAL_RESET_ERROR";
    g_mBugCheck[0x0000014D] = L"EXCEPTION_SCOPE_INVALID";
    g_mBugCheck[0x0000014E] = L"SOC_CRITICAL_DEVICE_REMOVED";
    g_mBugCheck[0x0000014F] = L"PDC_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x00000150] = L"TCPIP_AOAC_NIC_ACTIVE_REFERENCE_LEAK";
    g_mBugCheck[0x00000151] = L"UNSUPPORTED_INSTRUCTION_MODE";
    g_mBugCheck[0x00000152] = L"INVALID_PUSH_LOCK_FLAGS";
    g_mBugCheck[0x00000153] = L"KERNEL_LOCK_ENTRY_LEAKED_ON_THREAD_TERMINATION";
    g_mBugCheck[0x00000154] = L"UNEXPECTED_STORE_EXCEPTION";
    g_mBugCheck[0x00000155] = L"OS_DATA_TAMPERING";
    g_mBugCheck[0x00000157] = L"KERNEL_THREAD_PRIORITY_FLOOR_VIOLATION";
    g_mBugCheck[0x00000158] = L"ILLEGAL_IOMMU_PAGE_FAULT";
    g_mBugCheck[0x00000159] = L"HAL_ILLEGAL_IOMMU_PAGE_FAULT";
    g_mBugCheck[0x0000015A] = L"SDBUS_INTERNAL_ERROR";
    g_mBugCheck[0x0000015B] = L"WORKER_THREAD_RETURNED_WITH_SYSTEM_PAGE_PRIORITY_ACTIVE";
    g_mBugCheck[0x00000160] = L"WIN32K_ATOMIC_CHECK_FAILURE";
    g_mBugCheck[0x00000162] = L"KERNEL_AUTO_BOOST_INVALID_LOCK_RELEASE";
    g_mBugCheck[0x00000163] = L"WORKER_THREAD_TEST_CONDITION";
    g_mBugCheck[0x00000164] = L"WIN32K_CRITICAL_FAILURE";
    g_mBugCheck[0x0000016C] = L"INVALID_RUNDOWN_PROTECTION_FLAGS";
    g_mBugCheck[0x0000016D] = L"INVALID_SLOT_ALLOCATOR_FLAGS";
    g_mBugCheck[0x0000016E] = L"ERESOURCE_INVALID_RELEASE";
    g_mBugCheck[0x00000170] = L"CLUSTER_CSV_CLUSSVC_DISCONNECT_WATCHDOG";
    g_mBugCheck[0x00000171] = L"CRYPTO_LIBRARY_INTERNAL_ERROR";
    g_mBugCheck[0x00000173] = L"COREMSGCALL_INTERNAL_ERROR";
    g_mBugCheck[0x00000174] = L"COREMSG_INTERNAL_ERROR";
    g_mBugCheck[0x00000178] = L"ELAM_DRIVER_DETECTED_FATAL_ERROR";
    g_mBugCheck[0x0000017B] = L"PROFILER_CONFIGURATION_ILLEGAL";
    g_mBugCheck[0x0000017E] = L"MICROCODE_REVISION_MISMATCH";
    g_mBugCheck[0x00000187] = L"VIDEO_DWMINIT_TIMEOUT_FALLBACK_BDD";
    g_mBugCheck[0x00000189] = L"BAD_OBJECT_HEADER";
    g_mBugCheck[0x0000018B] = L"SECURE_KERNEL_ERROR";
    g_mBugCheck[0x0000018C] = L"HYPERGUARD_VIOLATION";
    g_mBugCheck[0x0000018D] = L"SECURE_FAULT_UNHANDLED";
    g_mBugCheck[0x0000018E] = L"KERNEL_PARTITION_REFERENCE_VIOLATION";
    g_mBugCheck[0x00000191] = L"PF_DETECTED_CORRUPTION";
    g_mBugCheck[0x00000192] = L"KERNEL_AUTO_BOOST_LOCK_ACQUISITION_WITH_RAISED_IRQL";
    g_mBugCheck[0x00000196] = L"LOADER_ROLLBACK_DETECTED";
    g_mBugCheck[0x00000197] = L"WIN32K_SECURITY_FAILURE";
    g_mBugCheck[0x00000199] = L"KERNEL_STORAGE_SLOT_IN_USE";
    g_mBugCheck[0x0000019A] = L"WORKER_THREAD_RETURNED_WHILE_ATTACHED_TO_SILO";
    g_mBugCheck[0x0000019B] = L"TTM_FATAL_ERROR";
    g_mBugCheck[0x0000019C] = L"WIN32K_POWER_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001A0] = L"TTM_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001A2] = L"WIN32K_CALLOUT_WATCHDOG_BUGCHECK";
    g_mBugCheck[0x000001AA] = L"EXCEPTION_ON_INVALID_STACK";
    g_mBugCheck[0x000001AB] = L"UNWIND_ON_INVALID_STACK";
    g_mBugCheck[0x000001C6] = L"FAST_ERESOURCE_PRECONDITION_VIOLATION";
    g_mBugCheck[0x000001C7] = L"STORE_DATA_STRUCTURE_CORRUPTION";
    g_mBugCheck[0x000001C8] = L"MANUALLY_INITIATED_POWER_BUTTON_HOLD";
    g_mBugCheck[0x000001CA] = L"SYNTHETIC_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001CB] = L"INVALID_SILO_DETACH";
    g_mBugCheck[0x000001CD] = L"INVALID_CALLBACK_STACK_ADDRESS";
    g_mBugCheck[0x000001CE] = L"INVALID_KERNEL_STACK_ADDRESS";
    g_mBugCheck[0x000001CF] = L"HARDWARE_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001D0] = L"CPI_FIRMWARE_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001D2] = L"WORKER_THREAD_INVALID_STATE";
    g_mBugCheck[0x000001D3] = L"WFP_INVALID_OPERATION";
    g_mBugCheck[0x000001D5] = L"DRIVER_PNP_WATCHDOG";
    g_mBugCheck[0x000001D6] = L"WORKER_THREAD_RETURNED_WITH_NON_DEFAULT_WORKLOAD_CLASS";
    g_mBugCheck[0x000001D7] = L"EFS_FATAL_ERROR";
    g_mBugCheck[0x000001D8] = L"UCMUCSI_FAILURE";
    g_mBugCheck[0x000001D9] = L"HAL_IOMMU_INTERNAL_ERROR";
    g_mBugCheck[0x000001DA] = L"HAL_BLOCKED_PROCESSOR_INTERNAL_ERROR";
    g_mBugCheck[0x000001DB] = L"IPI_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001DC] = L"DMA_COMMON_BUFFER_VECTOR_ERROR";
    g_mBugCheck[0x000001DD] = L"BUGCODE_MBBADAPTER_DRIVER";
    g_mBugCheck[0x000001DE] = L"BUGCODE_WIFIADAPTER_DRIVER";
    g_mBugCheck[0x000001DF] = L"PROCESSOR_START_TIMEOUT";
    g_mBugCheck[0x000001E4] = L"VIDEO_DXGKRNL_SYSMM_FATAL_ERROR";
    g_mBugCheck[0x000001E9] = L"ILLEGAL_ATS_INITIALIZATION";
    g_mBugCheck[0x000001EA] = L"SECURE_PCI_CONFIG_SPACE_ACCESS_VIOLATION";
    g_mBugCheck[0x000001EB] = L"DAM_WATCHDOG_TIMEOUT";
    g_mBugCheck[0x000001ED] = L"HANDLE_ERROR_ON_CRITICAL_THREAD";
    g_mBugCheck[0x00000356] = L"XBOX_ERACTRL_CS_TIMEOUT";
    g_mBugCheck[0x00000BFE] = L"BC_BLUETOOTH_VERIFIER_FAULT";
    g_mBugCheck[0x00000BFF] = L"BC_BTHMINI_VERIFIER_FAULT";
    g_mBugCheck[0x00020001] = L"HYPERVISOR_ERROR";
    g_mBugCheck[0x1000007E] = L"SYSTEM_THREAD_EXCEPTION_NOT_HANDLED_M";
    g_mBugCheck[0x1000007F] = L"UNEXPECTED_KERNEL_MODE_TRAP_M";
    g_mBugCheck[0x1000008E] = L"KERNEL_MODE_EXCEPTION_NOT_HANDLED_M";
    g_mBugCheck[0x100000EA] = L"THREAD_STUCK_IN_DEVICE_DRIVER_M";
    g_mBugCheck[0x4000008A] = L"THREAD_TERMINATE_HELD_MUTEX";
    g_mBugCheck[0xC0000218] = L"STATUS_CANNOT_LOAD_REGISTRY_FILE";
    g_mBugCheck[0xC000021A] = L"WINLOGON_FATAL_ERROR";
    g_mBugCheck[0xC0000221] = L"STATUS_IMAGE_CHECKSUM_MISMATCH";
    g_mBugCheck[0xDEADDEAD] = L"MANUALLY_INITIATED_CRASH1";

}


/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: setLDAPCodes

  Summary:  Fill map with error code definitions for LDAP
            Based on C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\Winldap.h

            Q&D:
            Search \s*(\S*)\s*=\s*(.*),$
            Replace g_mLDAP[\2] = L"\1";\n

  Args:

  Returns:

-----------------------------------------------------------------F-F*/
void setLDAPCodes() {

    g_mLDAP[0x00] = L"LDAP_SUCCESS";
    g_mLDAP[0x01] = L"LDAP_OPERATIONS_ERROR";
    g_mLDAP[0x02] = L"LDAP_PROTOCOL_ERROR";
    g_mLDAP[0x03] = L"LDAP_TIMELIMIT_EXCEEDED";
    g_mLDAP[0x04] = L"LDAP_SIZELIMIT_EXCEEDED";
    g_mLDAP[0x05] = L"LDAP_COMPARE_FALSE";
    g_mLDAP[0x06] = L"LDAP_COMPARE_TRUE";
    g_mLDAP[0x07] = L"LDAP_AUTH_METHOD_NOT_SUPPORTED";
    g_mLDAP[0x08] = L"LDAP_STRONG_AUTH_REQUIRED";
    g_mLDAP[0x09] = L"LDAP_REFERRAL_V2";
    g_mLDAP[0x09] = L"LDAP_PARTIAL_RESULTS";
    g_mLDAP[0x0a] = L"LDAP_REFERRAL";
    g_mLDAP[0x0b] = L"LDAP_ADMIN_LIMIT_EXCEEDED";
    g_mLDAP[0x0c] = L"LDAP_UNAVAILABLE_CRIT_EXTENSION";
    g_mLDAP[0x0d] = L"LDAP_CONFIDENTIALITY_REQUIRED";
    g_mLDAP[0x0e] = L"LDAP_SASL_BIND_IN_PROGRESS";
    g_mLDAP[0x10] = L"LDAP_NO_SUCH_ATTRIBUTE";
    g_mLDAP[0x11] = L"LDAP_UNDEFINED_TYPE";
    g_mLDAP[0x12] = L"LDAP_INAPPROPRIATE_MATCHING";
    g_mLDAP[0x13] = L"LDAP_CONSTRAINT_VIOLATION";
    g_mLDAP[0x14] = L"LDAP_ATTRIBUTE_OR_VALUE_EXISTS";
    g_mLDAP[0x15] = L"LDAP_INVALID_SYNTAX";
    g_mLDAP[0x20] = L"LDAP_NO_SUCH_OBJECT";
    g_mLDAP[0x21] = L"LDAP_ALIAS_PROBLEM";
    g_mLDAP[0x22] = L"LDAP_INVALID_DN_SYNTAX";
    g_mLDAP[0x23] = L"LDAP_IS_LEAF";
    g_mLDAP[0x24] = L"LDAP_ALIAS_DEREF_PROBLEM";
    g_mLDAP[0x30] = L"LDAP_INAPPROPRIATE_AUTH";
    g_mLDAP[0x31] = L"LDAP_INVALID_CREDENTIALS";
    g_mLDAP[0x32] = L"LDAP_INSUFFICIENT_RIGHTS";
    g_mLDAP[0x33] = L"LDAP_BUSY";
    g_mLDAP[0x34] = L"LDAP_UNAVAILABLE";
    g_mLDAP[0x35] = L"LDAP_UNWILLING_TO_PERFORM";
    g_mLDAP[0x36] = L"LDAP_LOOP_DETECT";
    g_mLDAP[0x3C] = L"LDAP_SORT_CONTROL_MISSING";
    g_mLDAP[0x3D] = L"LDAP_OFFSET_RANGE_ERROR";
    g_mLDAP[0x40] = L"LDAP_NAMING_VIOLATION";
    g_mLDAP[0x41] = L"LDAP_OBJECT_CLASS_VIOLATION";
    g_mLDAP[0x42] = L"LDAP_NOT_ALLOWED_ON_NONLEAF";
    g_mLDAP[0x43] = L"LDAP_NOT_ALLOWED_ON_RDN";
    g_mLDAP[0x44] = L"LDAP_ALREADY_EXISTS";
    g_mLDAP[0x45] = L"LDAP_NO_OBJECT_CLASS_MODS";
    g_mLDAP[0x46] = L"LDAP_RESULTS_TOO_LARGE";
    g_mLDAP[0x47] = L"LDAP_AFFECTS_MULTIPLE_DSAS";
    g_mLDAP[0x4c] = L"LDAP_VIRTUAL_LIST_VIEW_ERROR";
    g_mLDAP[0x50] = L"LDAP_OTHER";
    g_mLDAP[0x51] = L"LDAP_SERVER_DOWN";
    g_mLDAP[0x52] = L"LDAP_LOCAL_ERROR";
    g_mLDAP[0x53] = L"LDAP_ENCODING_ERROR";
    g_mLDAP[0x54] = L"LDAP_DECODING_ERROR";
    g_mLDAP[0x55] = L"LDAP_TIMEOUT";
    g_mLDAP[0x56] = L"LDAP_AUTH_UNKNOWN";
    g_mLDAP[0x57] = L"LDAP_FILTER_ERROR";
    g_mLDAP[0x58] = L"LDAP_USER_CANCELLED";
    g_mLDAP[0x59] = L"LDAP_PARAM_ERROR";
    g_mLDAP[0x5a] = L"LDAP_NO_MEMORY";
    g_mLDAP[0x5b] = L"LDAP_CONNECT_ERROR";
    g_mLDAP[0x5c] = L"LDAP_NOT_SUPPORTED";
    g_mLDAP[0x5e] = L"LDAP_NO_RESULTS_RETURNED";
    g_mLDAP[0x5d] = L"LDAP_CONTROL_NOT_FOUND";
    g_mLDAP[0x5f] = L"LDAP_MORE_RESULTS_TO_RETURN";
    g_mLDAP[0x60] = L"LDAP_CLIENT_LOOP";
    g_mLDAP[0x61] = L"LDAP_REFERRAL_LIMIT_EXCEEDED";

}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: setWUCodes

  Summary:  Fill map with error code definitions for Windows Update 
            Based on C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\wuerror.h

            Q&D:
            Search \r\n^//.*$\r\n^.*MessageID: (.*)$\r\n^//.*$\r\n^//.*$\r\n^//.*$\r\n^// (.*)$\r\n^//.*$\r\n^.*_HRESULT_TYPEDEF_\((.*)\)$
            Replace g_mWU[\3] = L"\1\\r\\n\(\2\)";

  Args:

  Returns:

-----------------------------------------------------------------F-F*/
void setWUCodes() {

    //
    // Define the severity codes
    //

    g_mWU[0x00240001L] = L"WU_S_SERVICE_STOP\r\n(Windows Update Agent was stopped successfully)";
    g_mWU[0x00240002L] = L"WU_S_SELFUPDATE\r\n(Windows Update Agent updated itself)";
    g_mWU[0x00240003L] = L"WU_S_UPDATE_ERROR\r\n(Operation completed successfully but there were errors applying the updates)";
    g_mWU[0x00240004L] = L"WU_S_MARKED_FOR_DISCONNECT\r\n(A callback was marked to be disconnected later because the request to disconnect the operation came while a callback was executing)";
    g_mWU[0x00240005L] = L"WU_S_REBOOT_REQUIRED\r\n(The system must be restarted to complete installation of the update)";
    g_mWU[0x00240006L] = L"WU_S_ALREADY_INSTALLED\r\n(The update to be installed is already installed on the system)";
    g_mWU[0x00240007L] = L"WU_S_ALREADY_UNINSTALLED\r\n(The update to be removed is not installed on the system)";
    g_mWU[0x00240008L] = L"WU_S_ALREADY_DOWNLOADED\r\n(The update to be downloaded has already been downloaded)";
    g_mWU[0x00240009L] = L"WU_S_SOME_UPDATES_SKIPPED_ON_BATTERY\r\n(The operation completed successfully, but some updates were skipped because the system is running on batteries)";
    g_mWU[0x0024000AL] = L"WU_S_ALREADY_REVERTED\r\n(The update to be reverted is not present on the system)";
    g_mWU[0x00240010L] = L"WU_S_SEARCH_CRITERIA_NOT_SUPPORTED\r\n(The operation is skipped because the update service does not support the requested search criteria)";
    g_mWU[0x00242015L] = L"WU_S_UH_INSTALLSTILLPENDING\r\n(The installation operation for the update is still in progress)";
    g_mWU[0x00242016L] = L"WU_S_UH_DOWNLOAD_SIZE_CALCULATED\r\n(The actual download size has been calculated by the handler)";
    g_mWU[0x00245001L] = L"WU_S_SIH_NOOP\r\n(No operation was required by the server-initiated healing server response)";
    g_mWU[0x00246001L] = L"WU_S_DM_ALREADYDOWNLOADING\r\n(The update to be downloaded is already being downloaded)";
    g_mWU[0x00247101L] = L"WU_S_METADATA_SKIPPED_BY_ENFORCEMENTMODE\r\n(Metadata verification was skipped by enforcement mode)";
    g_mWU[0x00247102L] = L"WU_S_METADATA_IGNORED_SIGNATURE_VERIFICATION\r\n(A server configuration refresh resulted in metadata signature verification to be ignored)";
    g_mWU[0x00248001L] = L"WU_S_SEARCH_LOAD_SHEDDING\r\n(Search operation completed successfully but one or more services were shedding load)";
    g_mWU[0x00248002L] = L"WU_S_AAD_DEVICE_TICKET_NOT_NEEDED\r\n(There was no need to retrieve an AAD device ticket)";

    ///////////////////////////////////////////////////////////////////////////////
    // Windows Update Error Codes
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80240001L] = L"WU_E_NO_SERVICE\r\n(Windows Update Agent was unable to provide the service)";
    g_mWU[0x80240002L] = L"WU_E_MAX_CAPACITY_REACHED\r\n(The maximum capacity of the service was exceeded)";
    g_mWU[0x80240003L] = L"WU_E_UNKNOWN_ID\r\n(An ID cannot be found)";
    g_mWU[0x80240004L] = L"WU_E_NOT_INITIALIZED\r\n(The object could not be initialized)";
    g_mWU[0x80240005L] = L"WU_E_RANGEOVERLAP\r\n(The update handler requested a byte range overlapping a previously requested range)";
    g_mWU[0x80240006L] = L"WU_E_TOOMANYRANGES\r\n(The requested number of byte ranges exceeds the maximum number (2^31 - 1))";
    g_mWU[0x80240007L] = L"WU_E_INVALIDINDEX\r\n(The index to a collection was invalid)";
    g_mWU[0x80240008L] = L"WU_E_ITEMNOTFOUND\r\n(The key for the item queried could not be found)";
    g_mWU[0x80240009L] = L"WU_E_OPERATIONINPROGRESS\r\n(Another conflicting operation was in progress. Some operations such as installation cannot be performed twice simultaneously)";
    g_mWU[0x8024000AL] = L"WU_E_COULDNOTCANCEL\r\n(Cancellation of the operation was not allowed)";
    g_mWU[0x8024000BL] = L"WU_E_CALL_CANCELLED\r\n(Operation was cancelled)";
    g_mWU[0x8024000CL] = L"WU_E_NOOP\r\n(No operation was required)";
    g_mWU[0x8024000DL] = L"WU_E_XML_MISSINGDATA\r\n(Windows Update Agent could not find required information in the update's XML data)";
    g_mWU[0x8024000EL] = L"WU_E_XML_INVALID\r\n(Windows Update Agent found invalid information in the update's XML data)";
    g_mWU[0x8024000FL] = L"WU_E_CYCLE_DETECTED\r\n(Circular update relationships were detected in the metadata)";
    g_mWU[0x80240010L] = L"WU_E_TOO_DEEP_RELATION\r\n(Update relationships too deep to evaluate were evaluated)";
    g_mWU[0x80240011L] = L"WU_E_INVALID_RELATIONSHIP\r\n(An invalid update relationship was detected)";
    g_mWU[0x80240012L] = L"WU_E_REG_VALUE_INVALID\r\n(An invalid registry value was read)";
    g_mWU[0x80240013L] = L"WU_E_DUPLICATE_ITEM\r\n(Operation tried to add a duplicate item to a list)";
    g_mWU[0x80240014L] = L"WU_E_INVALID_INSTALL_REQUESTED\r\n(Updates requested for install are not installable by caller)";
    g_mWU[0x80240016L] = L"WU_E_INSTALL_NOT_ALLOWED\r\n(Operation tried to install while another installation was in progress or the system was pending a mandatory restart)";
    g_mWU[0x80240017L] = L"WU_E_NOT_APPLICABLE\r\n(Operation was not performed because there are no applicable updates)";
    g_mWU[0x80240018L] = L"WU_E_NO_USERTOKEN\r\n(Operation failed because a required user token is missing)";
    g_mWU[0x80240019L] = L"WU_E_EXCLUSIVE_INSTALL_CONFLICT\r\n(An exclusive update cannot be installed with other updates at the same time)";
    g_mWU[0x8024001AL] = L"WU_E_POLICY_NOT_SET\r\n(A policy value was not set)";
    g_mWU[0x8024001BL] = L"WU_E_SELFUPDATE_IN_PROGRESS\r\n(The operation could not be performed because the Windows Update Agent is self-updating)";
    g_mWU[0x8024001DL] = L"WU_E_INVALID_UPDATE\r\n(An update contains invalid metadata)";
    g_mWU[0x8024001EL] = L"WU_E_SERVICE_STOP\r\n(Operation did not complete because the service or system was being shut down)";
    g_mWU[0x8024001FL] = L"WU_E_NO_CONNECTION\r\n(Operation did not complete because the network connection was unavailable)";
    g_mWU[0x80240020L] = L"WU_E_NO_INTERACTIVE_USER\r\n(Operation did not complete because there is no logged-on interactive user)";
    g_mWU[0x80240021L] = L"WU_E_TIME_OUT\r\n(Operation did not complete because it timed out)";
    g_mWU[0x80240022L] = L"WU_E_ALL_UPDATES_FAILED\r\n(Operation failed for all the updates)";
    g_mWU[0x80240023L] = L"WU_E_EULAS_DECLINED\r\n(The license terms for all updates were declined)";
    g_mWU[0x80240024L] = L"WU_E_NO_UPDATE\r\n(There are no updates)";
    g_mWU[0x80240025L] = L"WU_E_USER_ACCESS_DISABLED\r\n(Group Policy settings prevented access to Windows Update)";
    g_mWU[0x80240026L] = L"WU_E_INVALID_UPDATE_TYPE\r\n(The type of update is invalid)";
    g_mWU[0x80240027L] = L"WU_E_URL_TOO_LONG\r\n(The URL exceeded the maximum length)";
    g_mWU[0x80240028L] = L"WU_E_UNINSTALL_NOT_ALLOWED\r\n(The update could not be uninstalled because the request did not originate from a WSUS server)";
    g_mWU[0x80240029L] = L"WU_E_INVALID_PRODUCT_LICENSE\r\n(Search may have missed some updates before there is an unlicensed application on the system)";
    g_mWU[0x8024002AL] = L"WU_E_MISSING_HANDLER\r\n(A component required to detect applicable updates was missing)";
    g_mWU[0x8024002BL] = L"WU_E_LEGACYSERVER\r\n(An operation did not complete because it requires a newer version of server)";
    g_mWU[0x8024002CL] = L"WU_E_BIN_SOURCE_ABSENT\r\n(A delta-compressed update could not be installed because it required the source)";
    g_mWU[0x8024002DL] = L"WU_E_SOURCE_ABSENT\r\n(A full-file update could not be installed because it required the source)";
    g_mWU[0x8024002EL] = L"WU_E_WU_DISABLED\r\n(Access to an unmanaged server is not allowed)";
    g_mWU[0x8024002FL] = L"WU_E_CALL_CANCELLED_BY_POLICY\r\n(Operation did not complete because the DisableWindowsUpdateAccess policy was set)";
    g_mWU[0x80240030L] = L"WU_E_INVALID_PROXY_SERVER\r\n(The format of the proxy list was invalid)";
    g_mWU[0x80240031L] = L"WU_E_INVALID_FILE\r\n(The file is in the wrong format)";
    g_mWU[0x80240032L] = L"WU_E_INVALID_CRITERIA\r\n(The search criteria string was invalid)";
    g_mWU[0x80240033L] = L"WU_E_EULA_UNAVAILABLE\r\n(License terms could not be downloaded)";
    g_mWU[0x80240034L] = L"WU_E_DOWNLOAD_FAILED\r\n(Update failed to download)";
    g_mWU[0x80240035L] = L"WU_E_UPDATE_NOT_PROCESSED\r\n(The update was not processed)";
    g_mWU[0x80240036L] = L"WU_E_INVALID_OPERATION\r\n(The object's current state did not allow the operation)";
    g_mWU[0x80240037L] = L"WU_E_NOT_SUPPORTED\r\n(The functionality for the operation is not supported)";
    g_mWU[0x80240038L] = L"WU_E_WINHTTP_INVALID_FILE\r\n(The downloaded file has an unexpected content type)";
    g_mWU[0x80240039L] = L"WU_E_TOO_MANY_RESYNC\r\n(Agent is asked by server to resync too many times)";
    g_mWU[0x80240040L] = L"WU_E_NO_SERVER_CORE_SUPPORT\r\n(WUA API method does not run on Server Core installation)";
    g_mWU[0x80240041L] = L"WU_E_SYSPREP_IN_PROGRESS\r\n(Service is not available while sysprep is running)";
    g_mWU[0x80240042L] = L"WU_E_UNKNOWN_SERVICE\r\n(The update service is no longer registered with AU)";
    g_mWU[0x80240043L] = L"WU_E_NO_UI_SUPPORT\r\n(There is no support for WUA UI)";
    g_mWU[0x80240044L] = L"WU_E_PER_MACHINE_UPDATE_ACCESS_DENIED\r\n(Only administrators can perform this operation on per-machine updates)";
    g_mWU[0x80240045L] = L"WU_E_UNSUPPORTED_SEARCHSCOPE\r\n(A search was attempted with a scope that is not currently supported for this type of search)";
    g_mWU[0x80240046L] = L"WU_E_BAD_FILE_URL\r\n(The URL does not point to a file)";
    g_mWU[0x80240047L] = L"WU_E_REVERT_NOT_ALLOWED\r\n(The update could not be reverted)";
    g_mWU[0x80240048L] = L"WU_E_INVALID_NOTIFICATION_INFO\r\n(The featured update notification info returned by the server is invalid)";
    g_mWU[0x80240049L] = L"WU_E_OUTOFRANGE\r\n(The data is out of range)";
    g_mWU[0x8024004AL] = L"WU_E_SETUP_IN_PROGRESS\r\n(Windows Update agent operations are not available while OS setup is running)";
    g_mWU[0x8024004BL] = L"WU_E_ORPHANED_DOWNLOAD_JOB\r\n(An orphaned downloadjob was found with no active callers)";
    g_mWU[0x8024004CL] = L"WU_E_LOW_BATTERY\r\n(An update could not be installed because the system battery power level is too low)";
    g_mWU[0x8024004DL] = L"WU_E_INFRASTRUCTUREFILE_INVALID_FORMAT\r\n(The downloaded infrastructure file is incorrectly formatted)";
    g_mWU[0x8024004EL] = L"WU_E_INFRASTRUCTUREFILE_REQUIRES_SSL\r\n(The infrastructure file must be downloaded using strong SSL)";
    g_mWU[0x8024004FL] = L"WU_E_IDLESHUTDOWN_OPCOUNT_DISCOVERY\r\n(A discovery call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240050L] = L"WU_E_IDLESHUTDOWN_OPCOUNT_SEARCH\r\n(A search call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240051L] = L"WU_E_IDLESHUTDOWN_OPCOUNT_DOWNLOAD\r\n(A download call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240052L] = L"WU_E_IDLESHUTDOWN_OPCOUNT_INSTALL\r\n(An install call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240053L] = L"WU_E_IDLESHUTDOWN_OPCOUNT_OTHER\r\n(An unspecified call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240054L] = L"WU_E_INTERACTIVE_CALL_CANCELLED\r\n(An interactive user cancelled this operation, which was started from the Windows Update Agent UI)";
    g_mWU[0x80240055L] = L"WU_E_AU_CALL_CANCELLED\r\n(Automatic Updates cancelled this operation because it applies to an update that is no longer applicable to this computer)";
    g_mWU[0x80240056L] = L"WU_E_SYSTEM_UNSUPPORTED\r\n(This version or edition of the operating system doesn't support the needed functionality)";
    g_mWU[0x80240057L] = L"WU_E_NO_SUCH_HANDLER_PLUGIN\r\n(The requested update download or install handler, or update applicability expression evaluator, is not provided by this Agent plugin)";
    g_mWU[0x80240058L] = L"WU_E_INVALID_SERIALIZATION_VERSION\r\n(The requested serialization version is not supported)";
    g_mWU[0x80240059L] = L"WU_E_NETWORK_COST_EXCEEDS_POLICY\r\n(The current network cost does not meet the conditions set by the network cost policy)";
    g_mWU[0x8024005AL] = L"WU_E_CALL_CANCELLED_BY_HIDE\r\n(The call is cancelled because it applies to an update that is hidden (no longer applicable to this computer))";
    g_mWU[0x8024005BL] = L"WU_E_CALL_CANCELLED_BY_INVALID\r\n(The call is cancelled because it applies to an update that is invalid (no longer applicable to this computer))";
    g_mWU[0x8024005CL] = L"WU_E_INVALID_VOLUMEID\r\n(The specified volume id is invalid)";
    g_mWU[0x8024005DL] = L"WU_E_UNRECOGNIZED_VOLUMEID\r\n(The specified volume id is unrecognized by the system)";
    g_mWU[0x8024005EL] = L"WU_E_EXTENDEDERROR_NOTSET\r\n(The installation extended error code is not specified)";
    g_mWU[0x8024005FL] = L"WU_E_EXTENDEDERROR_FAILED\r\n(The installation extended error code is set to general fail)";
    g_mWU[0x80240060L] = L"WU_E_IDLESHUTDOWN_OPCOUNT_SERVICEREGISTRATION\r\n(A service registration call contributed to a non-zero operation count at idle timer shutdown)";
    g_mWU[0x80240061L] = L"WU_E_FILETRUST_SHA2SIGNATURE_MISSING\r\n(Signature validation of the file fails to find valid SHA2+ signature on MS signed payload)";
    g_mWU[0x80240062L] = L"WU_E_UPDATE_NOT_APPROVED\r\n(The update is not in the servicing approval list)";
    g_mWU[0x80240063L] = L"WU_E_CALL_CANCELLED_BY_INTERACTIVE_SEARCH\r\n(The search call was cancelled by another interactive search against the same service)";
    g_mWU[0x80240064L] = L"WU_E_INSTALL_JOB_RESUME_NOT_ALLOWED\r\n(Resume of install job not allowed due to another installation in progress)";
    g_mWU[0x80240065L] = L"WU_E_INSTALL_JOB_NOT_SUSPENDED\r\n(Resume of install job not allowed because job is not suspended)";
    g_mWU[0x80240066L] = L"WU_E_INSTALL_USERCONTEXT_ACCESSDENIED\r\n(User context passed to installation from caller with insufficient privileges)";
    g_mWU[0x80240067L] = L"WU_E_STANDBY_ACTIVITY_NOT_ALLOWED\r\n(Operation is not allowed because the device is in DC (Direct Current) and DS (Disconnected Standby))";
    g_mWU[0x80240068L] = L"WU_E_COULD_NOT_EVALUATE_PROPERTY\r\n(The property could not be evaluated)";
    g_mWU[0x80240FFFL] = L"WU_E_UNEXPECTED\r\n(An operation failed due to reasons not covered by another error code)";

    ///////////////////////////////////////////////////////////////////////////////
    // Windows Installer minor errors
    //
    // The following errors are used to indicate that part of a search failed for
    // MSI problems. Another part of the search may successfully return updates.
    // All MSI minor codes should share the same error code range so that the caller
    // tell that they are related to Windows Installer.
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80241001L] = L"WU_E_MSI_WRONG_VERSION\r\n(Search may have missed some updates because the Windows Installer is less than version 3.1)";
    g_mWU[0x80241002L] = L"WU_E_MSI_NOT_CONFIGURED\r\n(Search may have missed some updates because the Windows Installer is not configured)";
    g_mWU[0x80241003L] = L"WU_E_MSP_DISABLED\r\n(Search may have missed some updates because policy has disabled Windows Installer patching)";
    g_mWU[0x80241004L] = L"WU_E_MSI_WRONG_APP_CONTEXT\r\n(An update could not be applied because the application is installed per-user)";
    g_mWU[0x80241005L] = L"WU_E_MSI_NOT_PRESENT\r\n(Search may have missed some updates because the Windows Installer is less than version 3.1)";
    g_mWU[0x80241FFFL] = L"WU_E_MSP_UNEXPECTED\r\n(Search may have missed some updates because there was a failure of the Windows Installer)";

    ///////////////////////////////////////////////////////////////////////////////
    // Protocol Talker errors
    //
    // The following map to SOAPCLIENT_ERRORs from atlsoap.h. These errors
    // are obtained from calling GetClientError() on the CClientWebService
    // object.
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80244000L] = L"WU_E_PT_SOAPCLIENT_BASE\r\n(WU_E_PT_SOAPCLIENT_* error codes map to the SOAPCLIENT_ERROR enum of the ATL Server Library)";
    g_mWU[0x80244001L] = L"WU_E_PT_SOAPCLIENT_INITIALIZE\r\n(Same as SOAPCLIENT_INITIALIZE_ERROR - initialization of the SOAP client failed, possibly because of an MSXML installation failure)";
    g_mWU[0x80244002L] = L"WU_E_PT_SOAPCLIENT_OUTOFMEMORY\r\n(Same as SOAPCLIENT_OUTOFMEMORY - SOAP client failed because it ran out of memory)";
    g_mWU[0x80244003L] = L"WU_E_PT_SOAPCLIENT_GENERATE\r\n(Same as SOAPCLIENT_GENERATE_ERROR - SOAP client failed to generate the request)";
    g_mWU[0x80244004L] = L"WU_E_PT_SOAPCLIENT_CONNECT\r\n(Same as SOAPCLIENT_CONNECT_ERROR - SOAP client failed to connect to the server)";
    g_mWU[0x80244005L] = L"WU_E_PT_SOAPCLIENT_SEND\r\n(Same as SOAPCLIENT_SEND_ERROR - SOAP client failed to send a message for reasons of WU_E_WINHTTP_* error codes)";
    g_mWU[0x80244006L] = L"WU_E_PT_SOAPCLIENT_SERVER\r\n(Same as SOAPCLIENT_SERVER_ERROR - SOAP client failed because there was a server error)";
    g_mWU[0x80244007L] = L"WU_E_PT_SOAPCLIENT_SOAPFAULT\r\n(Same as SOAPCLIENT_SOAPFAULT - SOAP client failed because there was a SOAP fault for reasons of WU_E_PT_SOAP_* error codes)";
    g_mWU[0x80244008L] = L"WU_E_PT_SOAPCLIENT_PARSEFAULT\r\n(Same as SOAPCLIENT_PARSEFAULT_ERROR - SOAP client failed to parse a SOAP fault)";
    g_mWU[0x80244009L] = L"WU_E_PT_SOAPCLIENT_READ\r\n(Same as SOAPCLIENT_READ_ERROR - SOAP client failed while reading the response from the server)";
    g_mWU[0x8024400AL] = L"WU_E_PT_SOAPCLIENT_PARSE\r\n(Same as SOAPCLIENT_PARSE_ERROR - SOAP client failed to parse the response from the server)";

    // The following map to SOAP_ERROR_CODEs from atlsoap.h. These errors
    // are obtained from the m_fault.m_soapErrCode member on the
    // CClientWebService object when GetClientError() returned
    // SOAPCLIENT_SOAPFAULT.
    g_mWU[0x8024400BL] = L"WU_E_PT_SOAP_VERSION\r\n(Same as SOAP_E_VERSION_MISMATCH - SOAP client found an unrecognizable namespace for the SOAP envelope)";
    g_mWU[0x8024400CL] = L"WU_E_PT_SOAP_MUST_UNDERSTAND\r\n(Same as SOAP_E_MUST_UNDERSTAND - SOAP client was unable to understand a header)";
    g_mWU[0x8024400DL] = L"WU_E_PT_SOAP_CLIENT\r\n(Same as SOAP_E_CLIENT - SOAP client found the message was malformed; fix before resending)";
    g_mWU[0x8024400EL] = L"WU_E_PT_SOAP_SERVER\r\n(Same as SOAP_E_SERVER - The SOAP message could not be processed due to a server error; resend later)";
    g_mWU[0x8024400FL] = L"WU_E_PT_WMI_ERROR\r\n(There was an unspecified Windows Management Instrumentation (WMI) error)";
    g_mWU[0x80244010L] = L"WU_E_PT_EXCEEDED_MAX_SERVER_TRIPS\r\n(The number of round trips to the server exceeded the maximum limit)";
    g_mWU[0x80244011L] = L"WU_E_PT_SUS_SERVER_NOT_SET\r\n(WUServer policy value is missing in the registry)";
    g_mWU[0x80244012L] = L"WU_E_PT_DOUBLE_INITIALIZATION\r\n(Initialization failed because the object was already initialized)";
    g_mWU[0x80244013L] = L"WU_E_PT_INVALID_COMPUTER_NAME\r\n(The computer name could not be determined)";
    g_mWU[0x80244015L] = L"WU_E_PT_REFRESH_CACHE_REQUIRED\r\n(The reply from the server indicates that the server was changed or the cookie was invalid; refresh the state of the internal cache and retry)";
    g_mWU[0x80244016L] = L"WU_E_PT_HTTP_STATUS_BAD_REQUEST\r\n(Same as HTTP status 400 - the server could not process the request due to invalid syntax)";
    g_mWU[0x80244017L] = L"WU_E_PT_HTTP_STATUS_DENIED\r\n(Same as HTTP status 401 - the requested resource requires user authentication)";
    g_mWU[0x80244018L] = L"WU_E_PT_HTTP_STATUS_FORBIDDEN\r\n(Same as HTTP status 403 - server understood the request, but declined to fulfill it)";
    g_mWU[0x80244019L] = L"WU_E_PT_HTTP_STATUS_NOT_FOUND\r\n(Same as HTTP status 404 - the server cannot find the requested URI (Uniform Resource Identifier))";
    g_mWU[0x8024401AL] = L"WU_E_PT_HTTP_STATUS_BAD_METHOD\r\n(Same as HTTP status 405 - the HTTP method is not allowed)";
    g_mWU[0x8024401BL] = L"WU_E_PT_HTTP_STATUS_PROXY_AUTH_REQ\r\n(Same as HTTP status 407 - proxy authentication is required)";
    g_mWU[0x8024401CL] = L"WU_E_PT_HTTP_STATUS_REQUEST_TIMEOUT\r\n(Same as HTTP status 408 - the server timed out waiting for the request)";
    g_mWU[0x8024401DL] = L"WU_E_PT_HTTP_STATUS_CONFLICT\r\n(Same as HTTP status 409 - the request was not completed due to a conflict with the current state of the resource)";
    g_mWU[0x8024401EL] = L"WU_E_PT_HTTP_STATUS_GONE\r\n(Same as HTTP status 410 - requested resource is no longer available at the server)";
    g_mWU[0x8024401FL] = L"WU_E_PT_HTTP_STATUS_SERVER_ERROR\r\n(Same as HTTP status 500 - an error internal to the server prevented fulfilling the request)";
    g_mWU[0x80244020L] = L"WU_E_PT_HTTP_STATUS_NOT_SUPPORTED\r\n(Same as HTTP status 500 - server does not support the functionality required to fulfill the request)";
    g_mWU[0x80244021L] = L"WU_E_PT_HTTP_STATUS_BAD_GATEWAY\r\n(Same as HTTP status 502 - the server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request)";
    g_mWU[0x80244022L] = L"WU_E_PT_HTTP_STATUS_SERVICE_UNAVAIL\r\n(Same as HTTP status 503 - the service is temporarily overloaded)";
    g_mWU[0x80244023L] = L"WU_E_PT_HTTP_STATUS_GATEWAY_TIMEOUT\r\n(Same as HTTP status 503 - the request was timed out waiting for a gateway)";
    g_mWU[0x80244024L] = L"WU_E_PT_HTTP_STATUS_VERSION_NOT_SUP\r\n(Same as HTTP status 505 - the server does not support the HTTP protocol version used for the request)";
    g_mWU[0x80244025L] = L"WU_E_PT_FILE_LOCATIONS_CHANGED\r\n(Operation failed due to a changed file location; refresh internal state and resend)";
    g_mWU[0x80244026L] = L"WU_E_PT_REGISTRATION_NOT_SUPPORTED\r\n(Operation failed because Windows Update Agent does not support registration with a non-WSUS server)";
    g_mWU[0x80244027L] = L"WU_E_PT_NO_AUTH_PLUGINS_REQUESTED\r\n(The server returned an empty authentication information list)";
    g_mWU[0x80244028L] = L"WU_E_PT_NO_AUTH_COOKIES_CREATED\r\n(Windows Update Agent was unable to create any valid authentication cookies)";
    g_mWU[0x80244029L] = L"WU_E_PT_INVALID_CONFIG_PROP\r\n(A configuration property value was wrong)";
    g_mWU[0x8024402AL] = L"WU_E_PT_CONFIG_PROP_MISSING\r\n(A configuration property value was missing)";
    g_mWU[0x8024402BL] = L"WU_E_PT_HTTP_STATUS_NOT_MAPPED\r\n(The HTTP request could not be completed and the reason did not correspond to any of the WU_E_PT_HTTP_* error codes)";
    g_mWU[0x8024402CL] = L"WU_E_PT_WINHTTP_NAME_NOT_RESOLVED\r\n(Same as ERROR_WINHTTP_NAME_NOT_RESOLVED - the proxy server or target server name cannot be resolved)";
    g_mWU[0x8024402DL] = L"WU_E_PT_LOAD_SHEDDING\r\n(The server is shedding load)";
    g_mWU[0x8024402EL] = L"WU_E_PT_CLIENT_ENFORCED_LOAD_SHEDDING\r\n(Windows Update Agent is enforcing honoring the service load shedding interval)";
    g_mWU[0x8024502DL] = L"WU_E_PT_SAME_REDIR_ID\r\n(Windows Update Agent failed to download a redirector cabinet file with a new redirectorId value from the server during the recovery)";
    g_mWU[0x8024502EL] = L"WU_E_PT_NO_MANAGED_RECOVER\r\n(A redirector recovery action did not complete because the server is managed)";
    g_mWU[0x8024402FL] = L"WU_E_PT_ECP_SUCCEEDED_WITH_ERRORS\r\n(External cab file processing completed with some errors)";
    g_mWU[0x80244030L] = L"WU_E_PT_ECP_INIT_FAILED\r\n(The external cab processor initialization did not complete)";
    g_mWU[0x80244031L] = L"WU_E_PT_ECP_INVALID_FILE_FORMAT\r\n(The format of a metadata file was invalid)";
    g_mWU[0x80244032L] = L"WU_E_PT_ECP_INVALID_METADATA\r\n(External cab processor found invalid metadata)";
    g_mWU[0x80244033L] = L"WU_E_PT_ECP_FAILURE_TO_EXTRACT_DIGEST\r\n(The file digest could not be extracted from an external cab file)";
    g_mWU[0x80244034L] = L"WU_E_PT_ECP_FAILURE_TO_DECOMPRESS_CAB_FILE\r\n(An external cab file could not be decompressed)";
    g_mWU[0x80244035L] = L"WU_E_PT_ECP_FILE_LOCATION_ERROR\r\n(External cab processor was unable to get file locations)";
    g_mWU[0x80240436L] = L"WU_E_PT_CATALOG_SYNC_REQUIRED\r\n(The server does not support category-specific search; Full catalog search has to be issued instead)";
    g_mWU[0x80240437L] = L"WU_E_PT_SECURITY_VERIFICATION_FAILURE\r\n(There was a problem authorizing with the service)";
    g_mWU[0x80240438L] = L"WU_E_PT_ENDPOINT_UNREACHABLE\r\n(There is no route or network connectivity to the endpoint)";
    g_mWU[0x80240439L] = L"WU_E_PT_INVALID_FORMAT\r\n(The data received does not meet the data contract expectations)";
    g_mWU[0x8024043AL] = L"WU_E_PT_INVALID_URL\r\n(The url is invalid)";
    g_mWU[0x8024043BL] = L"WU_E_PT_NWS_NOT_LOADED\r\n(Unable to load NWS runtime)";
    g_mWU[0x8024043CL] = L"WU_E_PT_PROXY_AUTH_SCHEME_NOT_SUPPORTED\r\n(The proxy auth scheme is not supported)";
    g_mWU[0x8024043DL] = L"WU_E_SERVICEPROP_NOTAVAIL\r\n(The requested service property is not available)";
    g_mWU[0x8024043EL] = L"WU_E_PT_ENDPOINT_REFRESH_REQUIRED\r\n(The endpoint provider plugin requires online refresh)";
    g_mWU[0x8024043FL] = L"WU_E_PT_ENDPOINTURL_NOTAVAIL\r\n(A URL for the requested service endpoint is not available)";
    g_mWU[0x80240440L] = L"WU_E_PT_ENDPOINT_DISCONNECTED\r\n(The connection to the service endpoint died)";
    g_mWU[0x80240441L] = L"WU_E_PT_INVALID_OPERATION\r\n(The operation is invalid because protocol talker is in an inappropriate state)";
    g_mWU[0x80240442L] = L"WU_E_PT_OBJECT_FAULTED\r\n(The object is in a faulted state due to a previous error)";
    g_mWU[0x80240443L] = L"WU_E_PT_NUMERIC_OVERFLOW\r\n(The operation would lead to numeric overflow)";
    g_mWU[0x80240444L] = L"WU_E_PT_OPERATION_ABORTED\r\n(The operation was aborted)";
    g_mWU[0x80240445L] = L"WU_E_PT_OPERATION_ABANDONED\r\n(The operation was abandoned)";
    g_mWU[0x80240446L] = L"WU_E_PT_QUOTA_EXCEEDED\r\n(A quota was exceeded)";
    g_mWU[0x80240447L] = L"WU_E_PT_NO_TRANSLATION_AVAILABLE\r\n(The information was not available in the specified language)";
    g_mWU[0x80240448L] = L"WU_E_PT_ADDRESS_IN_USE\r\n(The address is already being used)";
    g_mWU[0x80240449L] = L"WU_E_PT_ADDRESS_NOT_AVAILABLE\r\n(The address is not valid for this context)";
    g_mWU[0x8024044AL] = L"WU_E_PT_OTHER\r\n(Unrecognized error occurred in the Windows Web Services framework)";
    g_mWU[0x8024044BL] = L"WU_E_PT_SECURITY_SYSTEM_FAILURE\r\n(A security operation failed in the Windows Web Services framework)";
    g_mWU[0x80244100L] = L"WU_E_PT_DATA_BOUNDARY_RESTRICTED\r\n(The client is data boundary restricted and needs to talk to a restricted endpoint)";
    g_mWU[0x80244101L] = L"WU_E_PT_GENERAL_AAD_CLIENT_ERROR\r\n(The client hit an error in retrieving AAD device ticket)";
    g_mWU[0x80244FFFL] = L"WU_E_PT_UNEXPECTED\r\n(A communication error not covered by another WU_E_PT_* error code)";

    ///////////////////////////////////////////////////////////////////////////////
    // Redirector errors
    //
    // The following errors are generated by the components that download and
    // parse the wuredir.cab
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80245001L] = L"WU_E_REDIRECTOR_LOAD_XML\r\n(The redirector XML document could not be loaded into the DOM class)";
    g_mWU[0x80245002L] = L"WU_E_REDIRECTOR_S_FALSE\r\n(The redirector XML document is missing some required information)";
    g_mWU[0x80245003L] = L"WU_E_REDIRECTOR_ID_SMALLER\r\n(The redirectorId in the downloaded redirector cab is less than in the cached cab)";
    g_mWU[0x80245004L] = L"WU_E_REDIRECTOR_UNKNOWN_SERVICE\r\n(The service ID is not supported in the service environment)";
    g_mWU[0x80245005L] = L"WU_E_REDIRECTOR_UNSUPPORTED_CONTENTTYPE\r\n(The response from the redirector server had an unsupported content type)";
    g_mWU[0x80245006L] = L"WU_E_REDIRECTOR_INVALID_RESPONSE\r\n(The response from the redirector server had an error status or was invalid)";
    g_mWU[0x80245008L] = L"WU_E_REDIRECTOR_ATTRPROVIDER_EXCEEDED_MAX_NAMEVALUE\r\n(The maximum number of name value pairs was exceeded by the attribute provider)";
    g_mWU[0x80245009L] = L"WU_E_REDIRECTOR_ATTRPROVIDER_INVALID_NAME\r\n(The name received from the attribute provider was invalid)";
    g_mWU[0x8024500AL] = L"WU_E_REDIRECTOR_ATTRPROVIDER_INVALID_VALUE\r\n(The value received from the attribute provider was invalid)";
    g_mWU[0x8024500BL] = L"WU_E_REDIRECTOR_SLS_GENERIC_ERROR\r\n(There was an error in connecting to or parsing the response from the Service Locator Service redirector server)";
    g_mWU[0x8024500CL] = L"WU_E_REDIRECTOR_CONNECT_POLICY\r\n(Connections to the redirector server are disallowed by managed policy)";
    g_mWU[0x8024500DL] = L"WU_E_REDIRECTOR_ONLINE_DISALLOWED\r\n(The redirector would go online but is disallowed by caller configuration)";
    g_mWU[0x802450FFL] = L"WU_E_REDIRECTOR_UNEXPECTED\r\n(The redirector failed for reasons not covered by another WU_E_REDIRECTOR_* error code)";

    ///////////////////////////////////////////////////////////////////////////////
    // SIH errors
    //
    // The following errors are generated by the components that are involved with
    // service-initiated healing.
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80245101L] = L"WU_E_SIH_VERIFY_DOWNLOAD_ENGINE\r\n(Verification of the servicing engine package failed)";
    g_mWU[0x80245102L] = L"WU_E_SIH_VERIFY_DOWNLOAD_PAYLOAD\r\n(Verification of a servicing package failed)";
    g_mWU[0x80245103L] = L"WU_E_SIH_VERIFY_STAGE_ENGINE\r\n(Verification of the staged engine failed)";
    g_mWU[0x80245104L] = L"WU_E_SIH_VERIFY_STAGE_PAYLOAD\r\n(Verification of a staged payload failed)";
    g_mWU[0x80245105L] = L"WU_E_SIH_ACTION_NOT_FOUND\r\n(An internal error occurred where the servicing action was not found)";
    g_mWU[0x80245106L] = L"WU_E_SIH_SLS_PARSE\r\n(There was a parse error in the service environment response)";
    g_mWU[0x80245107L] = L"WU_E_SIH_INVALIDHASH\r\n(A downloaded file failed an integrity check)";
    g_mWU[0x80245108L] = L"WU_E_SIH_NO_ENGINE\r\n(No engine was provided by the server-initiated healing server response)";
    g_mWU[0x80245109L] = L"WU_E_SIH_POST_REBOOT_INSTALL_FAILED\r\n(Post-reboot install failed)";
    g_mWU[0x8024510AL] = L"WU_E_SIH_POST_REBOOT_NO_CACHED_SLS_RESPONSE\r\n(There were pending reboot actions, but cached SLS response was not found post-reboot)";
    g_mWU[0x8024510BL] = L"WU_E_SIH_PARSE\r\n(Parsing command line arguments failed)";
    g_mWU[0x8024510CL] = L"WU_E_SIH_SECURITY\r\n(Security check failed)";
    g_mWU[0x8024510DL] = L"WU_E_SIH_PPL\r\n(PPL check failed)";
    g_mWU[0x8024510EL] = L"WU_E_SIH_POLICY\r\n(Execution was disabled by policy)";
    g_mWU[0x8024510FL] = L"WU_E_SIH_STDEXCEPTION\r\n(A standard exception was caught)";
    g_mWU[0x80245110L] = L"WU_E_SIH_NONSTDEXCEPTION\r\n(A non-standard exception was caught)";
    g_mWU[0x80245111L] = L"WU_E_SIH_ENGINE_EXCEPTION\r\n(The server-initiated healing engine encountered an exception not covered by another WU_E_SIH_* error code)";
    g_mWU[0x80245112L] = L"WU_E_SIH_BLOCKED_FOR_PLATFORM\r\n(You are running SIH Client with cmd not supported on your platform)";
    g_mWU[0x80245113L] = L"WU_E_SIH_ANOTHER_INSTANCE_RUNNING\r\n(Another SIH Client is already running)";
    g_mWU[0x80245114L] = L"WU_E_SIH_DNSRESILIENCY_OFF\r\n(Disable DNS resiliency feature per service configuration)";
    g_mWU[0x802451FFL] = L"WU_E_SIH_UNEXPECTED\r\n(There was a failure for reasons not covered by another WU_E_SIH_* error code)";

    ///////////////////////////////////////////////////////////////////////////////
    // driver util errors
    //
    // The device PnP enumerated device was pruned from the SystemSpec because
    // one of the hardware or compatible IDs matched an installed printer driver.
    // This is not considered a fatal error and the device is simply skipped.
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024C001L] = L"WU_E_DRV_PRUNED\r\n(A driver was skipped)";
    g_mWU[0x8024C002L] = L"WU_E_DRV_NOPROP_OR_LEGACY\r\n(A property for the driver could not be found. It may not conform with required specifications)";
    g_mWU[0x8024C003L] = L"WU_E_DRV_REG_MISMATCH\r\n(The registry type read for the driver does not match the expected type)";
    g_mWU[0x8024C004L] = L"WU_E_DRV_NO_METADATA\r\n(The driver update is missing metadata)";
    g_mWU[0x8024C005L] = L"WU_E_DRV_MISSING_ATTRIBUTE\r\n(The driver update is missing a required attribute)";
    g_mWU[0x8024C006L] = L"WU_E_DRV_SYNC_FAILED\r\n(Driver synchronization failed)";
    g_mWU[0x8024C007L] = L"WU_E_DRV_NO_PRINTER_CONTENT\r\n(Information required for the synchronization of applicable printers is missing)";
    g_mWU[0x8024C008L] = L"WU_E_DRV_DEVICE_PROBLEM\r\n(After installing a driver update, the updated device has reported a problem)";

    // MessageId 0xCE00 through 0xCEFF are reserved for post-install driver problem codes
    // (see uhdriver.cpp)
    g_mWU[0x8024CFFFL] = L"WU_E_DRV_UNEXPECTED\r\n(A driver error not covered by another WU_E_DRV_* code)";

    //////////////////////////////////////////////////////////////////////////////
    // data store errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80248000L] = L"WU_E_DS_SHUTDOWN\r\n(An operation failed because Windows Update Agent is shutting down)";
    g_mWU[0x80248001L] = L"WU_E_DS_INUSE\r\n(An operation failed because the data store was in use)";
    g_mWU[0x80248002L] = L"WU_E_DS_INVALID\r\n(The current and expected states of the data store do not match)";
    g_mWU[0x80248003L] = L"WU_E_DS_TABLEMISSING\r\n(The data store is missing a table)";
    g_mWU[0x80248004L] = L"WU_E_DS_TABLEINCORRECT\r\n(The data store contains a table with unexpected columns)";
    g_mWU[0x80248005L] = L"WU_E_DS_INVALIDTABLENAME\r\n(A table could not be opened because the table is not in the data store)";
    g_mWU[0x80248006L] = L"WU_E_DS_BADVERSION\r\n(The current and expected versions of the data store do not match)";
    g_mWU[0x80248007L] = L"WU_E_DS_NODATA\r\n(The information requested is not in the data store)";
    g_mWU[0x80248008L] = L"WU_E_DS_MISSINGDATA\r\n(The data store is missing required information or has a NULL in a table column that requires a non-null value)";
    g_mWU[0x80248009L] = L"WU_E_DS_MISSINGREF\r\n(The data store is missing required information or has a reference to missing license terms, file, localized property or linked row)";
    g_mWU[0x8024800AL] = L"WU_E_DS_UNKNOWNHANDLER\r\n(The update was not processed because its update handler could not be recognized)";
    g_mWU[0x8024800BL] = L"WU_E_DS_CANTDELETE\r\n(The update was not deleted because it is still referenced by one or more services)";
    g_mWU[0x8024800CL] = L"WU_E_DS_LOCKTIMEOUTEXPIRED\r\n(The data store section could not be locked within the allotted time)";
    g_mWU[0x8024800DL] = L"WU_E_DS_NOCATEGORIES\r\n(The category was not added because it contains no parent categories and is not a top-level category itself)";
    g_mWU[0x8024800EL] = L"WU_E_DS_ROWEXISTS\r\n(The row was not added because an existing row has the same primary key)";
    g_mWU[0x8024800FL] = L"WU_E_DS_STOREFILELOCKED\r\n(The data store could not be initialized because it was locked by another process)";
    g_mWU[0x80248010L] = L"WU_E_DS_CANNOTREGISTER\r\n(The data store is not allowed to be registered with COM in the current process)";
    g_mWU[0x80248011L] = L"WU_E_DS_UNABLETOSTART\r\n(Could not create a data store object in another process)";
    g_mWU[0x80248013L] = L"WU_E_DS_DUPLICATEUPDATEID\r\n(The server sent the same update to the client with two different revision IDs)";
    g_mWU[0x80248014L] = L"WU_E_DS_UNKNOWNSERVICE\r\n(An operation did not complete because the service is not in the data store)";
    g_mWU[0x80248015L] = L"WU_E_DS_SERVICEEXPIRED\r\n(An operation did not complete because the registration of the service has expired)";
    g_mWU[0x80248016L] = L"WU_E_DS_DECLINENOTALLOWED\r\n(A request to hide an update was declined because it is a mandatory update or because it was deployed with a deadline)";
    g_mWU[0x80248017L] = L"WU_E_DS_TABLESESSIONMISMATCH\r\n(A table was not closed because it is not associated with the session)";
    g_mWU[0x80248018L] = L"WU_E_DS_SESSIONLOCKMISMATCH\r\n(A table was not closed because it is not associated with the session)";
    g_mWU[0x80248019L] = L"WU_E_DS_NEEDWINDOWSSERVICE\r\n(A request to remove the Windows Update service or to unregister it with Automatic Updates was declined because it is a built-in service and/or Automatic Updates cannot fall back to another service)";
    g_mWU[0x8024801AL] = L"WU_E_DS_INVALIDOPERATION\r\n(A request was declined because the operation is not allowed)";
    g_mWU[0x8024801BL] = L"WU_E_DS_SCHEMAMISMATCH\r\n(The schema of the current data store and the schema of a table in a backup XML document do not match)";
    g_mWU[0x8024801CL] = L"WU_E_DS_RESETREQUIRED\r\n(The data store requires a session reset; release the session and retry with a new session)";
    g_mWU[0x8024801DL] = L"WU_E_DS_IMPERSONATED\r\n(A data store operation did not complete because it was requested with an impersonated identity)";
    g_mWU[0x8024801EL] = L"WU_E_DS_DATANOTAVAILABLE\r\n(An operation against update metadata did not complete because the data was never received from server)";
    g_mWU[0x8024801FL] = L"WU_E_DS_DATANOTLOADED\r\n(An operation against update metadata did not complete because the data was available but not loaded from datastore)";
    g_mWU[0x80248020L] = L"WU_E_DS_NODATA_NOSUCHREVISION\r\n(A data store operation did not complete because no such update revision is known)";
    g_mWU[0x80248021L] = L"WU_E_DS_NODATA_NOSUCHUPDATE\r\n(A data store operation did not complete because no such update is known)";
    g_mWU[0x80248022L] = L"WU_E_DS_NODATA_EULA\r\n(A data store operation did not complete because an update's EULA information is missing)";
    g_mWU[0x80248023L] = L"WU_E_DS_NODATA_SERVICE\r\n(A data store operation did not complete because a service's information is missing)";
    g_mWU[0x80248024L] = L"WU_E_DS_NODATA_COOKIE\r\n(A data store operation did not complete because a service's synchronization information is missing)";
    g_mWU[0x80248025L] = L"WU_E_DS_NODATA_TIMER\r\n(A data store operation did not complete because a timer's information is missing)";
    g_mWU[0x80248026L] = L"WU_E_DS_NODATA_CCR\r\n(A data store operation did not complete because a download's information is missing)";
    g_mWU[0x80248027L] = L"WU_E_DS_NODATA_FILE\r\n(A data store operation did not complete because a file's information is missing)";
    g_mWU[0x80248028L] = L"WU_E_DS_NODATA_DOWNLOADJOB\r\n(A data store operation did not complete because a download job's information is missing)";
    g_mWU[0x80248029L] = L"WU_E_DS_NODATA_TMI\r\n(A data store operation did not complete because a service's timestamp information is missing)";
    g_mWU[0x80248FFFL] = L"WU_E_DS_UNEXPECTED\r\n(A data store error not covered by another WU_E_DS_* code)";

    /////////////////////////////////////////////////////////////////////////////
    //Inventory Errors
    /////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80249001L] = L"WU_E_INVENTORY_PARSEFAILED\r\n(Parsing of the rule file failed)";
    g_mWU[0x80249002L] = L"WU_E_INVENTORY_GET_INVENTORY_TYPE_FAILED\r\n(Failed to get the requested inventory type from the server)";
    g_mWU[0x80249003L] = L"WU_E_INVENTORY_RESULT_UPLOAD_FAILED\r\n(Failed to upload inventory result to the server)";
    g_mWU[0x80249004L] = L"WU_E_INVENTORY_UNEXPECTED\r\n(There was an inventory error not covered by another error code)";
    g_mWU[0x80249005L] = L"WU_E_INVENTORY_WMI_ERROR\r\n(A WMI error occurred when enumerating the instances for a particular class)";

    /////////////////////////////////////////////////////////////////////////////
    //AU Errors
    /////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024A000L] = L"WU_E_AU_NOSERVICE\r\n(Automatic Updates was unable to service incoming requests)";
    g_mWU[0x8024A002L] = L"WU_E_AU_NONLEGACYSERVER\r\n(The old version of the Automatic Updates client has stopped because the WSUS server has been upgraded)";
    g_mWU[0x8024A003L] = L"WU_E_AU_LEGACYCLIENTDISABLED\r\n(The old version of the Automatic Updates client was disabled)";
    g_mWU[0x8024A004L] = L"WU_E_AU_PAUSED\r\n(Automatic Updates was unable to process incoming requests because it was paused)";
    g_mWU[0x8024A005L] = L"WU_E_AU_NO_REGISTERED_SERVICE\r\n(No unmanaged service is registered with AU)";
    g_mWU[0x8024A006L] = L"WU_E_AU_DETECT_SVCID_MISMATCH\r\n(The default service registered with AU changed during the search)";
    g_mWU[0x8024A007L] = L"WU_E_REBOOT_IN_PROGRESS\r\n(A reboot is in progress)";
    g_mWU[0x8024A008L] = L"WU_E_AU_OOBE_IN_PROGRESS\r\n(Automatic Updates can't process incoming requests while Windows Welcome is running)";
    g_mWU[0x8024AFFFL] = L"WU_E_AU_UNEXPECTED\r\n(An Automatic Updates error not covered by another WU_E_AU * code)";

    //////////////////////////////////////////////////////////////////////////////
    // update handler errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80242000L] = L"WU_E_UH_REMOTEUNAVAILABLE\r\n(A request for a remote update handler could not be completed because no remote process is available)";
    g_mWU[0x80242001L] = L"WU_E_UH_LOCALONLY\r\n(A request for a remote update handler could not be completed because the handler is local only)";
    g_mWU[0x80242002L] = L"WU_E_UH_UNKNOWNHANDLER\r\n(A request for an update handler could not be completed because the handler could not be recognized)";
    g_mWU[0x80242003L] = L"WU_E_UH_REMOTEALREADYACTIVE\r\n(A remote update handler could not be created because one already exists)";
    g_mWU[0x80242004L] = L"WU_E_UH_DOESNOTSUPPORTACTION\r\n(A request for the handler to install (uninstall) an update could not be completed because the update does not support install (uninstall))";
    g_mWU[0x80242005L] = L"WU_E_UH_WRONGHANDLER\r\n(An operation did not complete because the wrong handler was specified)";
    g_mWU[0x80242006L] = L"WU_E_UH_INVALIDMETADATA\r\n(A handler operation could not be completed because the update contains invalid metadata)";
    g_mWU[0x80242007L] = L"WU_E_UH_INSTALLERHUNG\r\n(An operation could not be completed because the installer exceeded the time limit)";
    g_mWU[0x80242008L] = L"WU_E_UH_OPERATIONCANCELLED\r\n(An operation being done by the update handler was cancelled)";
    g_mWU[0x80242009L] = L"WU_E_UH_BADHANDLERXML\r\n(An operation could not be completed because the handler-specific metadata is invalid)";
    g_mWU[0x8024200AL] = L"WU_E_UH_CANREQUIREINPUT\r\n(A request to the handler to install an update could not be completed because the update requires user input)";
    g_mWU[0x8024200BL] = L"WU_E_UH_INSTALLERFAILURE\r\n(The installer failed to install (uninstall) one or more updates)";
    g_mWU[0x8024200CL] = L"WU_E_UH_FALLBACKTOSELFCONTAINED\r\n(The update handler should download self-contained content rather than delta-compressed content for the update)";
    g_mWU[0x8024200DL] = L"WU_E_UH_NEEDANOTHERDOWNLOAD\r\n(The update handler did not install the update because it needs to be downloaded again)";
    g_mWU[0x8024200EL] = L"WU_E_UH_NOTIFYFAILURE\r\n(The update handler failed to send notification of the status of the install (uninstall) operation)";
    g_mWU[0x8024200FL] = L"WU_E_UH_INCONSISTENT_FILE_NAMES\r\n(The file names contained in the update metadata and in the update package are inconsistent)";
    g_mWU[0x80242010L] = L"WU_E_UH_FALLBACKERROR\r\n(The update handler failed to fall back to the self-contained content)";
    g_mWU[0x80242011L] = L"WU_E_UH_TOOMANYDOWNLOADREQUESTS\r\n(The update handler has exceeded the maximum number of download requests)";
    g_mWU[0x80242012L] = L"WU_E_UH_UNEXPECTEDCBSRESPONSE\r\n(The update handler has received an unexpected response from CBS)";
    g_mWU[0x80242013L] = L"WU_E_UH_BADCBSPACKAGEID\r\n(The update metadata contains an invalid CBS package identifier)";
    g_mWU[0x80242014L] = L"WU_E_UH_POSTREBOOTSTILLPENDING\r\n(The post-reboot operation for the update is still in progress)";
    g_mWU[0x80242015L] = L"WU_E_UH_POSTREBOOTRESULTUNKNOWN\r\n(The result of the post-reboot operation for the update could not be determined)";
    g_mWU[0x80242016L] = L"WU_E_UH_POSTREBOOTUNEXPECTEDSTATE\r\n(The state of the update after its post-reboot operation has completed is unexpected)";
    g_mWU[0x80242017L] = L"WU_E_UH_NEW_SERVICING_STACK_REQUIRED\r\n(The OS servicing stack must be updated before this update is downloaded or installed)";
    g_mWU[0x80242018L] = L"WU_E_UH_CALLED_BACK_FAILURE\r\n(A callback installer called back with an error)";
    g_mWU[0x80242019L] = L"WU_E_UH_CUSTOMINSTALLER_INVALID_SIGNATURE\r\n(The custom installer signature did not match the signature required by the update)";
    g_mWU[0x8024201AL] = L"WU_E_UH_UNSUPPORTED_INSTALLCONTEXT\r\n(The installer does not support the installation configuration)";
    g_mWU[0x8024201BL] = L"WU_E_UH_INVALID_TARGETSESSION\r\n(The targeted session for install is invalid)";
    g_mWU[0x8024201CL] = L"WU_E_UH_DECRYPTFAILURE\r\n(The handler failed to decrypt the update files)";
    g_mWU[0x8024201DL] = L"WU_E_UH_HANDLER_DISABLEDUNTILREBOOT\r\n(The update handler is disabled until the system reboots)";
    g_mWU[0x8024201EL] = L"WU_E_UH_APPX_NOT_PRESENT\r\n(The AppX infrastructure is not present on the system)";
    g_mWU[0x8024201FL] = L"WU_E_UH_NOTREADYTOCOMMIT\r\n(The update cannot be committed because it has not been previously installed or staged)";
    g_mWU[0x80242020L] = L"WU_E_UH_APPX_INVALID_PACKAGE_VOLUME\r\n(The specified volume is not a valid AppX package volume)";
    g_mWU[0x80242021L] = L"WU_E_UH_APPX_DEFAULT_PACKAGE_VOLUME_UNAVAILABLE\r\n(The configured default storage volume is unavailable)";
    g_mWU[0x80242022L] = L"WU_E_UH_APPX_INSTALLED_PACKAGE_VOLUME_UNAVAILABLE\r\n(The volume on which the application is installed is unavailable)";
    g_mWU[0x80242023L] = L"WU_E_UH_APPX_PACKAGE_FAMILY_NOT_FOUND\r\n(The specified package family is not present on the system)";
    g_mWU[0x80242024L] = L"WU_E_UH_APPX_SYSTEM_VOLUME_NOT_FOUND\r\n(Unable to find a package volume marked as system)";
    g_mWU[0x80242025L] = L"WU_E_UH_UA_SESSION_INFO_VERSION_NOT_SUPPORTED\r\n(UA does not support the version of OptionalSessionInfo)";
    g_mWU[0x80242026L] = L"WU_E_UH_RESERVICING_REQUIRED_BASELINE\r\n(This operation cannot be completed. You must install the baseline update(s) before you can install this update)";
    g_mWU[0x80242FFFL] = L"WU_E_UH_UNEXPECTED\r\n(An update handler error not covered by another WU_E_UH_* code)";

    //////////////////////////////////////////////////////////////////////////////
    // download manager errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80246001L] = L"WU_E_DM_URLNOTAVAILABLE\r\n(A download manager operation could not be completed because the requested file does not have a URL)";
    g_mWU[0x80246002L] = L"WU_E_DM_INCORRECTFILEHASH\r\n(A download manager operation could not be completed because the file digest was not recognized)";
    g_mWU[0x80246003L] = L"WU_E_DM_UNKNOWNALGORITHM\r\n(A download manager operation could not be completed because the file metadata requested an unrecognized hash algorithm)";
    g_mWU[0x80246004L] = L"WU_E_DM_NEEDDOWNLOADREQUEST\r\n(An operation could not be completed because a download request is required from the download handler)";
    g_mWU[0x80246005L] = L"WU_E_DM_NONETWORK\r\n(A download manager operation could not be completed because the network connection was unavailable)";
    g_mWU[0x80246006L] = L"WU_E_DM_WRONGBITSVERSION\r\n(A download manager operation could not be completed because the version of Background Intelligent Transfer Service (BITS) is incompatible)";
    g_mWU[0x80246007L] = L"WU_E_DM_NOTDOWNLOADED\r\n(The update has not been downloaded)";
    g_mWU[0x80246008L] = L"WU_E_DM_FAILTOCONNECTTOBITS\r\n(A download manager operation failed because the download manager was unable to connect the Background Intelligent Transfer Service (BITS))";
    g_mWU[0x80246009L] = L"WU_E_DM_BITSTRANSFERERROR\r\n(A download manager operation failed because there was an unspecified Background Intelligent Transfer Service (BITS) transfer error)";
    g_mWU[0x8024600AL] = L"WU_E_DM_DOWNLOADLOCATIONCHANGED\r\n(A download must be restarted because the location of the source of the download has changed)";
    g_mWU[0x8024600BL] = L"WU_E_DM_CONTENTCHANGED\r\n(A download must be restarted because the update content changed in a new revision)";
    g_mWU[0x8024600CL] = L"WU_E_DM_DOWNLOADLIMITEDBYUPDATESIZE\r\n(A download failed because the current network limits downloads by update size for the update service)";
    g_mWU[0x8024600EL] = L"WU_E_DM_UNAUTHORIZED\r\n(The download failed because the client was denied authorization to download the content)";
    g_mWU[0x8024600FL] = L"WU_E_DM_BG_ERROR_TOKEN_REQUIRED\r\n(The download failed because the user token associated with the BITS job no longer exists)";
    g_mWU[0x80246010L] = L"WU_E_DM_DOWNLOADSANDBOXNOTFOUND\r\n(The sandbox directory for the downloaded update was not found)";
    g_mWU[0x80246011L] = L"WU_E_DM_DOWNLOADFILEPATHUNKNOWN\r\n(The downloaded update has an unknown file path)";
    g_mWU[0x80246012L] = L"WU_E_DM_DOWNLOADFILEMISSING\r\n(One or more of the files for the downloaded update is missing)";
    g_mWU[0x80246013L] = L"WU_E_DM_UPDATEREMOVED\r\n(An attempt was made to access a downloaded update that has already been removed)";
    g_mWU[0x80246014L] = L"WU_E_DM_READRANGEFAILED\r\n(Windows Update couldn't find a needed portion of a downloaded update's file)";
    g_mWU[0x80246016L] = L"WU_E_DM_UNAUTHORIZED_NO_USER\r\n(The download failed because the client was denied authorization to download the content due to no user logged on)";
    g_mWU[0x80246017L] = L"WU_E_DM_UNAUTHORIZED_LOCAL_USER\r\n(The download failed because the local user was denied authorization to download the content)";
    g_mWU[0x80246018L] = L"WU_E_DM_UNAUTHORIZED_DOMAIN_USER\r\n(The download failed because the domain user was denied authorization to download the content)";
    g_mWU[0x80246019L] = L"WU_E_DM_UNAUTHORIZED_MSA_USER\r\n(The download failed because the MSA account associated with the user was denied authorization to download the content)";
    g_mWU[0x8024601AL] = L"WU_E_DM_FALLINGBACKTOBITS\r\n(The download will be continued by falling back to BITS to download the content)";
    g_mWU[0x8024601BL] = L"WU_E_DM_DOWNLOAD_VOLUME_CONFLICT\r\n(Another caller has requested download to a different volume)";
    g_mWU[0x8024601CL] = L"WU_E_DM_SANDBOX_HASH_MISMATCH\r\n(The hash of the update's sandbox does not match the expected value)";
    g_mWU[0x8024601DL] = L"WU_E_DM_HARDRESERVEID_CONFLICT\r\n(The hard reserve id specified conflicts with an id from another caller)";
    g_mWU[0x8024601EL] = L"WU_E_DM_DOSVC_REQUIRED\r\n(The update has to be downloaded via DO)";
    g_mWU[0x8024601FL] = L"WU_E_DM_DOWNLOADTYPE_CONFLICT\r\n(Windows Update only supports one download type per update at one time. The download failure is by design here since the same update with different download type is operating. Please try again later)";
    g_mWU[0x80246FFFL] = L"WU_E_DM_UNEXPECTED\r\n(There was a download manager error not covered by another WU_E_DM_* error code)";

    //////////////////////////////////////////////////////////////////////////////
    // Setup/SelfUpdate errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024D001L] = L"WU_E_SETUP_INVALID_INFDATA\r\n(Windows Update Agent could not be updated because an INF file contains invalid information)";
    g_mWU[0x8024D002L] = L"WU_E_SETUP_INVALID_IDENTDATA\r\n(Windows Update Agent could not be updated because the wuident.cab file contains invalid information)";
    g_mWU[0x8024D003L] = L"WU_E_SETUP_ALREADY_INITIALIZED\r\n(Windows Update Agent could not be updated because of an internal error that caused setup initialization to be performed twice)";
    g_mWU[0x8024D004L] = L"WU_E_SETUP_NOT_INITIALIZED\r\n(Windows Update Agent could not be updated because setup initialization never completed successfully)";
    g_mWU[0x8024D005L] = L"WU_E_SETUP_SOURCE_VERSION_MISMATCH\r\n(Windows Update Agent could not be updated because the versions specified in the INF do not match the actual source file versions)";
    g_mWU[0x8024D006L] = L"WU_E_SETUP_TARGET_VERSION_GREATER\r\n(Windows Update Agent could not be updated because a WUA file on the target system is newer than the corresponding source file)";
    g_mWU[0x8024D007L] = L"WU_E_SETUP_REGISTRATION_FAILED\r\n(Windows Update Agent could not be updated because regsvr32.exe returned an error)";
    g_mWU[0x8024D008L] = L"WU_E_SELFUPDATE_SKIP_ON_FAILURE\r\n(An update to the Windows Update Agent was skipped because previous attempts to update have failed)";
    g_mWU[0x8024D009L] = L"WU_E_SETUP_SKIP_UPDATE\r\n(An update to the Windows Update Agent was skipped due to a directive in the wuident.cab file)";
    g_mWU[0x8024D00AL] = L"WU_E_SETUP_UNSUPPORTED_CONFIGURATION\r\n(Windows Update Agent could not be updated because the current system configuration is not supported)";
    g_mWU[0x8024D00BL] = L"WU_E_SETUP_BLOCKED_CONFIGURATION\r\n(Windows Update Agent could not be updated because the system is configured to block the update)";
    g_mWU[0x8024D00CL] = L"WU_E_SETUP_REBOOT_TO_FIX\r\n(Windows Update Agent could not be updated because a restart of the system is required)";
    g_mWU[0x8024D00DL] = L"WU_E_SETUP_ALREADYRUNNING\r\n(Windows Update Agent setup is already running)";
    g_mWU[0x8024D00EL] = L"WU_E_SETUP_REBOOTREQUIRED\r\n(Windows Update Agent setup package requires a reboot to complete installation)";
    g_mWU[0x8024D00FL] = L"WU_E_SETUP_HANDLER_EXEC_FAILURE\r\n(Windows Update Agent could not be updated because the setup handler failed during execution)";
    g_mWU[0x8024D010L] = L"WU_E_SETUP_INVALID_REGISTRY_DATA\r\n(Windows Update Agent could not be updated because the registry contains invalid information)";
    g_mWU[0x8024D011L] = L"WU_E_SELFUPDATE_REQUIRED\r\n(Windows Update Agent must be updated before search can continue)";
    g_mWU[0x8024D012L] = L"WU_E_SELFUPDATE_REQUIRED_ADMIN\r\n(Windows Update Agent must be updated before search can continue.  An administrator is required to perform the operation)";
    g_mWU[0x8024D013L] = L"WU_E_SETUP_WRONG_SERVER_VERSION\r\n(Windows Update Agent could not be updated because the server does not contain update information for this version)";
    g_mWU[0x8024D014L] = L"WU_E_SETUP_DEFERRABLE_REBOOT_PENDING\r\n(Windows Update Agent is successfully updated, but a reboot is required to complete the setup)";
    g_mWU[0x8024D015L] = L"WU_E_SETUP_NON_DEFERRABLE_REBOOT_PENDING\r\n(Windows Update Agent is successfully updated, but a reboot is required to complete the setup)";
    g_mWU[0x8024D016L] = L"WU_E_SETUP_FAIL\r\n(Windows Update Agent could not be updated because of an unknown error)";
    g_mWU[0x8024DFFFL] = L"WU_E_SETUP_UNEXPECTED\r\n(Windows Update Agent could not be updated because of an error not covered by another WU_E_SETUP_* error code)";

    //////////////////////////////////////////////////////////////////////////////
    // expression evaluator errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024E001L] = L"WU_E_EE_UNKNOWN_EXPRESSION\r\n(An expression evaluator operation could not be completed because an expression was unrecognized)";
    g_mWU[0x8024E002L] = L"WU_E_EE_INVALID_EXPRESSION\r\n(An expression evaluator operation could not be completed because an expression was invalid)";
    g_mWU[0x8024E003L] = L"WU_E_EE_MISSING_METADATA\r\n(An expression evaluator operation could not be completed because an expression contains an incorrect number of metadata nodes)";
    g_mWU[0x8024E004L] = L"WU_E_EE_INVALID_VERSION\r\n(An expression evaluator operation could not be completed because the version of the serialized expression data is invalid)";
    g_mWU[0x8024E005L] = L"WU_E_EE_NOT_INITIALIZED\r\n(The expression evaluator could not be initialized)";
    g_mWU[0x8024E006L] = L"WU_E_EE_INVALID_ATTRIBUTEDATA\r\n(An expression evaluator operation could not be completed because there was an invalid attribute)";
    g_mWU[0x8024E007L] = L"WU_E_EE_CLUSTER_ERROR\r\n(An expression evaluator operation could not be completed because the cluster state of the computer could not be determined)";
    g_mWU[0x8024EFFFL] = L"WU_E_EE_UNEXPECTED\r\n(There was an expression evaluator error not covered by another WU_E_EE_* error code)";

    //////////////////////////////////////////////////////////////////////////////
    // UI errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x80243001L] = L"WU_E_INSTALLATION_RESULTS_UNKNOWN_VERSION\r\n(The results of download and installation could not be read from the registry due to an unrecognized data format version)";
    g_mWU[0x80243002L] = L"WU_E_INSTALLATION_RESULTS_INVALID_DATA\r\n(The results of download and installation could not be read from the registry due to an invalid data format)";
    g_mWU[0x80243003L] = L"WU_E_INSTALLATION_RESULTS_NOT_FOUND\r\n(The results of download and installation are not available; the operation may have failed to start)";
    g_mWU[0x80243004L] = L"WU_E_TRAYICON_FAILURE\r\n(A failure occurred when trying to create an icon in the taskbar notification area)";
    g_mWU[0x80243FFDL] = L"WU_E_NON_UI_MODE\r\n(Unable to show UI when in non-UI mode; WU client UI modules may not be installed)";
    g_mWU[0x80243FFEL] = L"WU_E_WUCLTUI_UNSUPPORTED_VERSION\r\n(Unsupported version of WU client UI exported functions)";
    g_mWU[0x80243FFFL] = L"WU_E_AUCLIENT_UNEXPECTED\r\n(There was a user interface error not covered by another WU_E_AUCLIENT_* error code)";

    //////////////////////////////////////////////////////////////////////////////
    // reporter errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024F001L] = L"WU_E_REPORTER_EVENTCACHECORRUPT\r\n(The event cache file was defective)";
    g_mWU[0x8024F002L] = L"WU_E_REPORTER_EVENTNAMESPACEPARSEFAILED\r\n(The XML in the event namespace descriptor could not be parsed)";
    g_mWU[0x8024F003L] = L"WU_E_INVALID_EVENT\r\n(The XML in the event namespace descriptor could not be parsed)";
    g_mWU[0x8024F004L] = L"WU_E_SERVER_BUSY\r\n(The server rejected an event because the server was too busy)";
    g_mWU[0x8024F005L] = L"WU_E_CALLBACK_COOKIE_NOT_FOUND\r\n(The specified callback cookie is not found)";
    g_mWU[0x8024FFFFL] = L"WU_E_REPORTER_UNEXPECTED\r\n(There was a reporter error not covered by another error code)";
    g_mWU[0x80247001L] = L"WU_E_OL_INVALID_SCANFILE\r\n(An operation could not be completed because the scan package was invalid)";
    g_mWU[0x80247002L] = L"WU_E_OL_NEWCLIENT_REQUIRED\r\n(An operation could not be completed because the scan package requires a greater version of the Windows Update Agent)";
    g_mWU[0x80247003L] = L"WU_E_INVALID_EVENT_PAYLOAD\r\n(An invalid event payload was specified)";
    g_mWU[0x80247004L] = L"WU_E_INVALID_EVENT_PAYLOADSIZE\r\n(The size of the event payload submitted is invalid)";
    g_mWU[0x80247005L] = L"WU_E_SERVICE_NOT_REGISTERED\r\n(The service is not registered)";
    g_mWU[0x80247FFFL] = L"WU_E_OL_UNEXPECTED\r\n(Search using the scan package failed)";

    //////////////////////////////////////////////////////////////////////////////
    // WU Metadata Integrity related errors - 0x71FE
    ///////////////////////////////////////////////////////////////////////////////
    ///////
    // Metadata General errors 0x7100 - 0x711F
    ///////
    g_mWU[0x80247100L] = L"WU_E_METADATA_NOOP\r\n(No operation was required by update metadata verification)";
    g_mWU[0x80247101L] = L"WU_E_METADATA_CONFIG_INVALID_BINARY_ENCODING\r\n(The binary encoding of metadata config data was invalid)";
    g_mWU[0x80247102L] = L"WU_E_METADATA_FETCH_CONFIG\r\n(Unable to fetch required configuration for metadata signature verification)";
    g_mWU[0x80247104L] = L"WU_E_METADATA_INVALID_PARAMETER\r\n(A metadata verification operation failed due to an invalid parameter)";
    g_mWU[0x80247105L] = L"WU_E_METADATA_UNEXPECTED\r\n(A metadata verification operation failed due to reasons not covered by another error code)";
    g_mWU[0x80247106L] = L"WU_E_METADATA_NO_VERIFICATION_DATA\r\n(None of the update metadata had verification data, which may be disabled on the update server)";
    g_mWU[0x80247107L] = L"WU_E_METADATA_BAD_FRAGMENTSIGNING_CONFIG\r\n(The fragment signing configuration used for verifying update metadata signatures was bad)";
    g_mWU[0x80247108L] = L"WU_E_METADATA_FAILURE_PROCESSING_FRAGMENTSIGNING_CONFIG\r\n(There was an unexpected operational failure while parsing fragment signing configuration)";

    ///////
    // Metadata XML errors 0x7120 - 0x713F
    ///////
    g_mWU[0x80247120L] = L"WU_E_METADATA_XML_MISSING\r\n(Required xml data was missing from configuration)";
    g_mWU[0x80247121L] = L"WU_E_METADATA_XML_FRAGMENTSIGNING_MISSING\r\n(Required fragmentsigning data was missing from xml configuration)";
    g_mWU[0x80247122L] = L"WU_E_METADATA_XML_MODE_MISSING\r\n(Required mode data was missing from xml configuration)";
    g_mWU[0x80247123L] = L"WU_E_METADATA_XML_MODE_INVALID\r\n(An invalid metadata enforcement mode was detected)";
    g_mWU[0x80247124L] = L"WU_E_METADATA_XML_VALIDITY_INVALID\r\n(An invalid timestamp validity window configuration was detected)";
    g_mWU[0x80247125L] = L"WU_E_METADATA_XML_LEAFCERT_MISSING\r\n(Required leaf certificate data was missing from xml configuration)";
    g_mWU[0x80247126L] = L"WU_E_METADATA_XML_INTERMEDIATECERT_MISSING\r\n(Required intermediate certificate data was missing from xml configuration)";
    g_mWU[0x80247127L] = L"WU_E_METADATA_XML_LEAFCERT_ID_MISSING\r\n(Required leaf certificate id attribute was missing from xml configuration)";
    g_mWU[0x80247128L] = L"WU_E_METADATA_XML_BASE64CERDATA_MISSING\r\n(Required certificate base64CerData attribute was missing from xml configuration)";

    ///////
    // Metadata Signature/Hash-related errors 0x7140 - 0x714F
    ///////
    g_mWU[0x80247140L] = L"WU_E_METADATA_BAD_SIGNATURE\r\n(The metadata for an update was found to have a bad or invalid digital signature)";
    g_mWU[0x80247141L] = L"WU_E_METADATA_UNSUPPORTED_HASH_ALG\r\n(An unsupported hash algorithm for metadata verification was specified)";
    g_mWU[0x80247142L] = L"WU_E_METADATA_SIGNATURE_VERIFY_FAILED\r\n(An error occurred during an update's metadata signature verification)";

    ///////
    // Metadata Certificate Chain trust related errors 0x7150 - 0x715F
    ///////
    g_mWU[0x80247150L] = L"WU_E_METADATATRUST_CERTIFICATECHAIN_VERIFICATION\r\n(An failure occurred while verifying trust for metadata signing certificate chains)";
    g_mWU[0x80247151L] = L"WU_E_METADATATRUST_UNTRUSTED_CERTIFICATECHAIN\r\n(A metadata signing certificate had an untrusted certificate chain)";

    ///////
    // Metadata Timestamp Token/Signature errors 0x7160 - 0x717F
    ///////
    g_mWU[0x80247160L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_MISSING\r\n(An expected metadata timestamp token was missing)";
    g_mWU[0x80247161L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_VERIFICATION_FAILED\r\n(A metadata Timestamp token failed verification)";
    g_mWU[0x80247162L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_UNTRUSTED\r\n(A metadata timestamp token signer certificate chain was untrusted)";
    g_mWU[0x80247163L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_VALIDITY_WINDOW\r\n(A metadata signature timestamp token was no longer within the validity window)";
    g_mWU[0x80247164L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_SIGNATURE\r\n(A metadata timestamp token failed signature validation)";
    g_mWU[0x80247165L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_CERTCHAIN\r\n(A metadata timestamp token certificate failed certificate chain verification)";
    g_mWU[0x80247166L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_REFRESHONLINE\r\n(A failure occurred when refreshing a missing timestamp token from the network)";
    g_mWU[0x80247167L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_ALL_BAD\r\n(All update metadata verification timestamp tokens from the timestamp token cache are invalid)";
    g_mWU[0x80247168L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_NODATA\r\n(No update metadata verification timestamp tokens exist in the timestamp token cache)";
    g_mWU[0x80247169L] = L"WU_E_METADATA_TIMESTAMP_TOKEN_CACHELOOKUP\r\n(An error occurred during cache lookup of update metadata verification timestamp token)";
    g_mWU[0x8024717EL] = L"WU_E_METADATA_TIMESTAMP_TOKEN_VALIDITYWINDOW_UNEXPECTED\r\n(An metadata timestamp token validity window failed unexpectedly due to reasons not covered by another error code)";
    g_mWU[0x8024717FL] = L"WU_E_METADATA_TIMESTAMP_TOKEN_UNEXPECTED\r\n(An metadata timestamp token verification operation failed due to reasons not covered by another error code)";

    ///////
    // Metadata Certificate-Related errors 0x7180 - 0x719F
    ///////
    g_mWU[0x80247180L] = L"WU_E_METADATA_CERT_MISSING\r\n(An expected metadata signing certificate was missing)";
    g_mWU[0x80247181L] = L"WU_E_METADATA_LEAFCERT_BAD_TRANSPORT_ENCODING\r\n(The transport encoding of a metadata signing leaf certificate was malformed)";
    g_mWU[0x80247182L] = L"WU_E_METADATA_INTCERT_BAD_TRANSPORT_ENCODING\r\n(The transport encoding of a metadata signing intermediate certificate was malformed)";
    g_mWU[0x80247183L] = L"WU_E_METADATA_CERT_UNTRUSTED\r\n(A metadata certificate chain was untrusted)";

    //////////////////////////////////////////////////////////////////////////////
    // WU Task related errors
    ///////////////////////////////////////////////////////////////////////////////
    g_mWU[0x8024B001L] = L"WU_E_WUTASK_INPROGRESS\r\n(The task is currently in progress)";
    g_mWU[0x8024B002L] = L"WU_E_WUTASK_STATUS_DISABLED\r\n(The operation cannot be completed since the task status is currently disabled)";
    g_mWU[0x8024B003L] = L"WU_E_WUTASK_NOT_STARTED\r\n(The operation cannot be completed since the task is not yet started)";
    g_mWU[0x8024B004L] = L"WU_E_WUTASK_RETRY\r\n(The task was stopped and needs to be run again to complete)";
    g_mWU[0x8024B005L] = L"WU_E_WUTASK_CANCELINSTALL_DISALLOWED\r\n(Cannot cancel a non-scheduled install)";

    //////////////////////////////////////////////////////////////////////////////
    // Hardware Capability related errors
    ////
    g_mWU[0x8024B101L] = L"WU_E_UNKNOWN_HARDWARECAPABILITY\r\n(Hardware capability meta data was not found after a sync with the service)";
    g_mWU[0x8024B102L] = L"WU_E_BAD_XML_HARDWARECAPABILITY\r\n(Hardware capability meta data was malformed and/or failed to parse)";
    g_mWU[0x8024B103L] = L"WU_E_WMI_NOT_SUPPORTED\r\n(Unable to complete action due to WMI dependency, which isn't supported on this platform)";
    g_mWU[0x8024B104L] = L"WU_E_UPDATE_MERGE_NOT_ALLOWED\r\n(Merging of the update is not allowed)";
    g_mWU[0x8024B105L] = L"WU_E_SKIPPED_UPDATE_INSTALLATION\r\n(Installing merged updates only. So skipping non mergeable updates)";

    //////////////////////////////////////////////////////////////////////////////
    // SLS related errors - 0xB201
    ////
    ///////
    // SLS General errors 0xB201 - 0xB2FF
    ///////
    g_mWU[0x8024B201L] = L"WU_E_SLS_INVALID_REVISION\r\n(SLS response returned invalid revision number)";

    //////////////////////////////////////////////////////////////////////////////
    // trust related errors - 0xB301
    ////
    ///////
    // trust General errors 0xB301 - 0xB3FF
    ///////
    g_mWU[0x8024B301L] = L"WU_E_FILETRUST_DUALSIGNATURE_RSA\r\n(File signature validation fails to find valid RSA signature on infrastructure payload)";
    g_mWU[0x8024B302L] = L"WU_E_FILETRUST_DUALSIGNATURE_ECC\r\n(File signature validation fails to find valid ECC signature on infrastructure payload)";
    g_mWU[0x8024B303L] = L"WU_E_TRUST_SUBJECT_NOT_TRUSTED\r\n(The subject is not trusted by WU for the specified action)";
    g_mWU[0x8024B304L] = L"WU_E_TRUST_PROVIDER_UNKNOWN\r\n(Unknown trust provider for WU)";

}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: wWinMain

  Summary:   Process window messages of main window

  Args:     HINSTANCE hInstance
            HINSTANCE hPrevInstance
            LPWSTR lpCmdLine
            int nCmdShow

  Returns:  int
              0 = success
              else error or canceled

-----------------------------------------------------------------F-F*/
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // Enables controls from Comctl32.dll, like status bar, tabs ...
    InitCommonControls();

    // Store instance
    g_hInst = hInstance;

    // Create list of windows update error codes
    setWUCodes();

    // Create list of LDAP error codes
    setLDAPCodes();

    // Create list of BugCheck error codes
    setBugCheckCodes();

    // Startr dialog
    return (int) DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, WndProcMainDialog);
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: SignedIntegerHexSubclassProc

  Summary:   Checks, if the cursor is at the beginning of the input edit control
             From https://devblogs.microsoft.com/oldnewthing/20190222-00/?p=101064

  Args:     HWND hEditControl
              Handle to input edit control

  Returns:  bool
              true = The cursor is at the beginning of the input edit control
              false = The cursor is NOT at the beginning of the input edit control

-----------------------------------------------------------------F-F*/
bool IsAtStartOfEditControl(HWND hEditControl)
{
    return (LOWORD(SendMessage(hEditControl, EM_GETSEL, 0, 0)) == 0);
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: SignedIntegerHexSubclassProc

  Summary:   Callback function to process messages for the input edit control (Use primary to prevent unsuppored chars)
             From https://devblogs.microsoft.com/oldnewthing/20190222-00/?p=101064

  Args:     HWND hEditControl
              Handle to input edit control
            UINT uMsg
              Message
            WPARAM wParam
            LPARAM lParam
            UINT_PTR uIdSubclass,
            DWORD_PTR dwRefData)

  Returns:  LRESULT

-----------------------------------------------------------------F-F*/
LRESULT CALLBACK SignedIntegerHexSubclassProc(
    HWND hEditControl,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    switch (uMsg) {
        case WM_NCDESTROY:
            RemoveWindowSubclass(hEditControl, SignedIntegerHexSubclassProc, uIdSubclass);
            break;
        case WM_CHAR:
        {
            wchar_t ch = (wchar_t)wParam;
            if (ch < L' ') break;                // let control character through
            else if ((ch == L'-' || ch == L'\x2212') && // hyphen-minus or Unicode minus sign
                IsAtStartOfEditControl(hEditControl)) break; // at start of edit control is okay
            else if (wcschr(L"0123456789xabcdefABCDEF", ch)) break;  // let digit/hex through
            MessageBeep(0);                      // otherwise invalid
            return 0;
        }
    }

    return DefSubclassProc(hEditControl, uMsg, wParam, lParam);
}

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: WndProcMainDialog

  Summary:   Process window messages of main dialog

  Args:     HWND hDlg
              Handle to main dialog
            UINT message
              Message
            WPARAM wParam
            LPARAM lParam

  Returns:  INT_PTR
              TRUE = Message was processed
              FALSE = Message was not processed

-----------------------------------------------------------------F-F*/
INT_PTR CALLBACK WndProcMainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
        case WM_INITDIALOG:
            {
                // Set properties for the input edit control
                HWND hInput = GetDlgItem(hDlg, IDC_INPUT);
                if (hInput != NULL) {
                    SetWindowSubclass(hInput, SignedIntegerHexSubclassProc, (WPARAM)0, (LPARAM)0); // Custom subclass callback
                    SendMessage(hInput, EM_LIMITTEXT, MAXVALUELENTH, 0); // Max chars
                    SendMessage(hInput, EM_SETCUEBANNER, 0, (LPARAM)LoadStringAsWstr(g_hInst, IDS_INPUTHINT).c_str()); // Textual cue/tip

                    // Get stored input value from registry
                    wchar_t szValue[MAXVALUELENTH + 1];
                    szValue[0] = L'\0';
                    DWORD valueSize = 0;
                    DWORD keyType = 0;
                    // Get size of registry value
                    if (RegGetValue(HKEY_CURRENT_USER, L"Software\\CodingABI\\TranslateErrorCode", L"LastInput", RRF_RT_REG_SZ, &keyType, NULL, &valueSize) == ERROR_SUCCESS) {
                        if ((valueSize > 0) && (valueSize <= (MAXVALUELENTH + 2) * sizeof(WCHAR))) { // Size OK?
                            // Get registry value
                            valueSize = (MAXVALUELENTH + 1) * sizeof(WCHAR); // Max size incl. termination
                            if (RegGetValue(HKEY_CURRENT_USER, L"Software\\CodingABI\\TranslateErrorCode", L"LastInput", RRF_RT_REG_SZ | RRF_ZEROONFAILURE, NULL, &szValue, &valueSize) == ERROR_SUCCESS) {
                                SetWindowText(hInput, szValue); // Set input control value to registry value
                            }
                        }
                    }
                }

                // Create tooltips
                std::wstring sResource;
                TOOLINFO ti;
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;

                // Add tooltip for button
                HWND hwndTooltipButton = CreateWindowEx(
                    0,
                    TOOLTIPS_CLASS,
                    L"",
                    TTS_ALWAYSTIP,
                    0, 0, 0, 0,
                    hDlg, 0, g_hInst, 0);

                HWND hButton = GetDlgItem(hDlg, IDC_BUTTONSEARCH);
                if (hButton != NULL) {
                    ti.uId = (UINT_PTR)hButton;
                    sResource.assign(LoadStringAsWstr(g_hInst, IDS_BUTTONTOOLTIP).c_str());
                    ti.lpszText = const_cast<wchar_t*>(sResource.c_str());
                    SendMessage(hwndTooltipButton, TTM_ADDTOOL, 0, (LPARAM)&ti);
                    if (isRunningUnderWine()) { // Wine has not uft zoom char 🔍 in font
                        SendMessage(hButton, WM_SETTEXT, 0, (LPARAM)L"►");
                    }
                }

                if (!isRunningUnderWine()) { // Wine has a tooltip/focus-bug for edit controls https://bugs.winehq.org/show_bug.cgi?id=41062
                    // Add tooltip for the input edit control
                    HWND hwndTooltipInput = CreateWindowEx(
                        0,
                        TOOLTIPS_CLASS,
                        L"",
                        TTS_ALWAYSTIP,
                        0, 0, 0, 0,
                        hDlg, 0, g_hInst, 0);

                    if (hInput != NULL) {
                        ti.uId = (UINT_PTR)hInput;
                        sResource.assign(LoadStringAsWstr(g_hInst, IDS_INPUTTOOLTIP).c_str());
                        ti.lpszText = const_cast<wchar_t*>(sResource.c_str());
                        SendMessage(hwndTooltipInput, TTM_ADDTOOL, 0, (LPARAM)&ti);
                    }
                }
                break;
            }
        case WM_CTLCOLORSTATIC:
            {
                // Change Textcolor/Background for output
                if ((HWND)lParam == GetDlgItem(hDlg, IDC_OUTPUT)) {
                    HDC hdcStatic = (HDC)wParam;
                    SetTextColor(hdcStatic, RGB(255, 255, 255));
                    SetBkColor(hdcStatic, RGB(0,116,129)); // Background of used lines
                    if (g_hbrOutputBackground == NULL) {
                        g_hbrOutputBackground = CreateSolidBrush(RGB(0, 116, 129));
                    }
                    return (INT_PTR)g_hbrOutputBackground;
                }
                break;
            }
        case WM_DESTROY:
            if (g_hbrOutputBackground != NULL) {
                DeleteObject(g_hbrOutputBackground);
                g_hbrOutputBackground = NULL;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL: // End dialog on window close or ESC
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                case IDOK: // Start translation on button press or ENTER
                case IDC_BUTTONSEARCH: {
                    HWND hInput = GetDlgItem(hDlg, IDC_INPUT);
                    if (hInput != NULL) {
                        wchar_t szValue[MAXVALUELENTH + 1];
                        GetWindowText(hInput, szValue, MAXVALUELENTH + 1); // Get value from input edit control
                        int iValue = 0;
                        if (StrToIntEx(szValue, STIF_SUPPORT_HEX, &iValue)) {
                            // Store input in registry
                            RegSetKeyValue(HKEY_CURRENT_USER, L"Software\\CodingABI\\TranslateErrorCode", L"LastInput", REG_SZ, szValue, (DWORD) (wcslen(szValue) + 1)*sizeof(WCHAR));
                            #define _MAX_ITOSTR_BASE16_COUNT (8 + 1) // Char length for DWORD to hex conversion
                            wchar_t szHex[_MAX_ITOSTR_BASE16_COUNT + 2];
                            std::wstring sMessage = L"";
                            LPWSTR messageBuffer = nullptr;
                            // Convert error to hex
                            _snwprintf_s(szHex, _MAX_ITOSTR_BASE16_COUNT + 2, _TRUNCATE, L"0x%08X", iValue);
                            // Numeric values
                            sMessage = std::wstring(L"DWORD \t").append(std::to_wstring((DWORD)iValue)).append(L"\r\n")
                                .append(L"int \t").append(std::to_wstring((int)iValue)).append(L"\r\n")
                                .append(L"Hex \t").append(szHex);
                            size_t size;

                            // Get message for Win32/HRESULT
                            size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, iValue, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);
                            if (size > 0) { // Append text from FormatMessage function, when error code was found in this function
                                StrTrim(messageBuffer, L"\n");
                                sMessage.append(L"\r\n\r\nWin32/HRESULT: ").append(messageBuffer);
                            }
                            LocalFree(messageBuffer);

                            // Get message for NTSTATUS
                            HMODULE hDLL = GetModuleHandle(L"ntdll.dll");
                            if (hDLL != NULL) {
                                size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                    GetModuleHandle(L"ntdll.dll"), iValue, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);
                                if (size > 0) { // Append text from FormatMessage function, when error code was found in this function
                                    StrTrim(messageBuffer, L"\r\n");
                                    sMessage.append(L"\r\n\r\nNTSTATUS: ").append(messageBuffer);
                                }
                            }
                            LocalFree(messageBuffer);

                            // Get message for windows update
                            if (g_mWU.count(iValue) > 0) { // Append text from windows update message, when error code was found in the messages
                                sMessage.append(L"\r\n\r\nWU: ").append(g_mWU[iValue]);
                            }
                            // Get message for LDAP
                            if (g_mLDAP.count(iValue) > 0) { // Append text from LDAP message, when error code was found in the messages
                                sMessage.append(L"\r\n\r\nLDAP: ").append(g_mLDAP[iValue]);
                            }
                            // Get message for stop codes
                            if (g_mBugCheck.count(iValue) > 0) { // Append text from BugCheck message, when error code was found in the messages
                                sMessage.append(L"\r\n\r\nStopCode/BugCheck: ").append(g_mBugCheck[iValue]);
                            }
                            HWND hOutput = GetDlgItem(hDlg, IDC_OUTPUT);
                            if (hOutput != NULL) {
                                SetWindowText(hOutput, sMessage.c_str()); // Set output text
                            }
                        }
                    }
                    break;
                }
            }
            break;
        case WM_NOTIFY :
            switch (((LPNMHDR)lParam)->code) {
                case NM_CLICK:          // Fall through to the next case.
                case NM_RETURN: {
                        // Open github link
                        PNMLINK pNMLink = (PNMLINK)lParam;
                        LITEM   item = pNMLink->item;

                        if ((((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hDlg, IDC_GITHUBLINK))) ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
                        break;
                    }
            }
            break;
    }
    return (INT_PTR)FALSE;
}