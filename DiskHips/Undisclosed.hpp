#pragma once
#include <windef.h>
#include "Includes.hpp"

#define GetArrayNumber(p) (sizeof(p)/sizeof((p)[0]))
#define PROCESS_TERMINATE         0x0001
#define PROCESS_VM_OPERATION      0x0008
#define PROCESS_VM_READ           0x0010
#define PROCESS_VM_WRITE          0x0020

#define OBJECT_TO_OBJECT_HEADER(o) CONTAINING_RECORD((o),OBJECT_HEADER,Body)

typedef NTSTATUS(__fastcall* MiProcessLoaderEntry)(PVOID pDriverSection, BOOLEAN bLoad);

typedef struct _AUX_ACCESS_DATA {
	PPRIVILEGE_SET PrivilegesUsed;
	GENERIC_MAPPING GenericMapping;
	ACCESS_MASK AccessesToAudit;
	ACCESS_MASK MaximumAuditMask;
	ULONG Unknown[256];
} AUX_ACCESS_DATA, * PAUX_ACCESS_DATA;

typedef struct _SYSTEM_MODULE_INFORMATION {
	HANDLE Section;
	PVOID MappedBase;
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT PathLength;
	CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,                 //  0 Y N
	SystemProcessorInformation,             //  1 Y N
	SystemPerformanceInformation,           //  2 Y N
	SystemTimeOfDayInformation,             //  3 Y N
	SystemNotImplemented1,                  //  4 Y N
	SystemProcessesAndThreadsInformation,   //  5 Y N
	SystemCallCounts,                       //  6 Y N
	SystemConfigurationInformation,         //  7 Y N
	SystemProcessorTimes,                   //  8 Y N
	SystemGlobalFlag,                       //  9 Y Y
	SystemNotImplemented2,                  // 10 Y N
	SystemModuleInformation,                // 11 Y N
	SystemLockInformation,                  // 12 Y N
	SystemNotImplemented3,                  // 13 Y N
	SystemNotImplemented4,                  // 14 Y N
	SystemNotImplemented5,                  // 15 Y N
	SystemHandleInformation,                // 16 Y N
	SystemObjectInformation,                // 17 Y N
	SystemPagefileInformation,              // 18 Y N
	SystemInstructionEmulationCounts,       // 19 Y N
	SystemInvalidInfoClass1,                // 20
	SystemCacheInformation,                 // 21 Y Y
	SystemPoolTagInformation,               // 22 Y N
	SystemProcessorStatistics,              // 23 Y N
	SystemDpcInformation,                   // 24 Y Y
	SystemNotImplemented6,                  // 25 Y N
	SystemLoadImage,                        // 26 N Y
	SystemUnloadImage,                      // 27 N Y
	SystemTimeAdjustment,                   // 28 Y Y
	SystemNotImplemented7,                  // 29 Y N
	SystemNotImplemented8,                  // 30 Y N
	SystemNotImplemented9,                  // 31 Y N
	SystemCrashDumpInformation,             // 32 Y N
	SystemExceptionInformation,             // 33 Y N
	SystemCrashDumpStateInformation,        // 34 Y Y/N
	SystemKernelDebuggerInformation,        // 35 Y N
	SystemContextSwitchInformation,         // 36 Y N
	SystemRegistryQuotaInformation,         // 37 Y Y
	SystemLoadAndCallImage,                 // 38 N Y
	SystemPrioritySeparation,               // 39 N Y
	SystemNotImplemented10,                 // 40 Y N
	SystemNotImplemented11,                 // 41 Y N
	SystemInvalidInfoClass2,                // 42
	SystemInvalidInfoClass3,                // 43
	SystemTimeZoneInformation,              // 44 Y N
	SystemLookasideInformation,             // 45 Y N
	SystemSetTimeSlipEvent,                 // 46 N Y
	SystemCreateSession,                    // 47 N Y
	SystemDeleteSession,                    // 48 N Y
	SystemInvalidInfoClass4,                // 49
	SystemRangeStartInformation,            // 50 Y N
	SystemVerifierInformation,              // 51 Y Y
	SystemAddVerifier,                      // 52 N Y
	SystemSessionProcessesInformation       // 53 Y N
} SYSTEM_INFORMATION_CLASS;

typedef enum _THREAD_STATE {
	StateInitialized,
	StateReady,
	StateRunning,
	StateStandby,
	StateTerminated,
	StateWait,
	StateTransition,
	StateUnknown
} THREAD_STATE;

typedef struct _SYSTEM_THREADS {
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER CreateTime;
	ULONG WaitTime;
	PVOID StartAddress;
	CLIENT_ID ClientId;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
	ULONG ContextSwitchCount;
	THREAD_STATE State;
	KWAIT_REASON WaitReason;
} SYSTEM_THREADS, * PSYSTEM_THREADS;

typedef struct _SYSTEM_PROCESSES {
	ULONG          NextEntryDelta;          //构成结构序列的偏移量；
	ULONG          ThreadCount;             //线程数目；
	ULONG          Reserved1[6];
	LARGE_INTEGER  CreateTime;              //创建时间；
	LARGE_INTEGER  UserTime;                //用户模式(Ring 3)的CPU时间；
	LARGE_INTEGER  KernelTime;              //内核模式(Ring 0)的CPU时间；
	UNICODE_STRING ProcessName;             //进程名称；
	KPRIORITY      BasePriority;            //进程优先权；
	HANDLE          ProcessId;               //进程标识符；
	ULONG          InheritedFromProcessId;  //父进程的标识符；
	ULONG          HandleCount;             //句柄数目；
	ULONG          Reserved2[2];
	VM_COUNTERS    VmCounters;              //虚拟存储器的结构，见下；
	IO_COUNTERS    IoCounters;              //IO计数结构，见下；
	SYSTEM_THREADS Threads[1];              //进程相关线程的结构数组，见下；
}SYSTEM_PROCESSES, * PSYSTEM_PROCESSES;

#ifndef _WIN64
#pragma pack(1)
#endif
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
#ifndef _WIN64
#pragma pack()
#endif

/*
typedef struct _LDR_DATA                           // 24 elements, 0xE0 bytes (sizeof)
{
	struct _LIST_ENTRY InLoadOrderLinks;           // 2 elements, 0x10 bytes (sizeof)
	struct _LIST_ENTRY InMemoryOrderLinks;         // 2 elements, 0x10 bytes (sizeof)
	struct _LIST_ENTRY InInitializationOrderLinks; // 2 elements, 0x10 bytes (sizeof)
	VOID* DllBase;
	VOID* EntryPoint;
	ULONG32      SizeOfImage;
#ifdef _WIN64
	UINT8        _PADDING0_[0x4];
#endif
	struct _UNICODE_STRING FullDllName;            // 3 elements, 0x10 bytes (sizeof)
	struct _UNICODE_STRING BaseDllName;            // 3 elements, 0x10 bytes (sizeof)
	ULONG32      Flags;
}LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
*/

typedef struct _OBJECT_TYPE_INITIALIZER {
	UINT16 Length;
	union {
		UINT8 ObjectTypeFlags;
		struct {
			UINT8 CaseInsensitive : 1;
			UINT8 UnnamedObjectsOnly : 1;
			UINT8 UseDefaultObject : 1;
			UINT8 SecurityRequired : 1;
			UINT8 MaintainHandleCount : 1;
			UINT8 MaintainTypeList : 1;
			UINT8 SupportsObjectCallbacks : 1;
		};
	};
	ULONG32 ObjectTypeCode;
	ULONG32 InvalidAttributes;
	struct _GENERIC_MAPPING GenericMapping;
	ULONG32 ValidAccessMask;
	ULONG32 RetainAccess;
	enum _POOL_TYPE PoolType;
	ULONG32 DefaultPagedPoolCharge;
	ULONG32 DefaultNonPagedPoolCharge;
	PVOID DumpProcedure;
	PVOID OpenProcedure;
	PVOID CloseProcedure;
	PVOID DeleteProcedure;
	PVOID ParseProcedure;
	PVOID SecurityProcedure;
	PVOID QueryNameProcedure;
	PVOID OkayToCloseProcedure;
}OBJECT_TYPE_INITIALIZER, * POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE_TEMP {
	struct _LIST_ENTRY TypeList;
	struct _UNICODE_STRING Name;
	PVOID DefaultObject;
	UINT8 Index;
	UINT8 _PADDING0_[0x3];
	ULONG32 TotalNumberOfObjects;
	ULONG32 TotalNumberOfHandles;
	ULONG32 HighWaterNumberOfObjects;
	ULONG32 HighWaterNumberOfHandles;
	UINT8 _PADDING1_[0x4];
	struct _OBJECT_TYPE_INITIALIZER TypeInfo;
	ULONG64 TypeLock;
	ULONG32 Key;
	UINT8 _PADDING2_[0x4];
	struct _LIST_ENTRY CallbackList;
}OBJECT_TYPE_TEMP, * POBJECT_TYPE_TEMP;

typedef enum _FIRMWARE_REENTRY {
	HalHaltRoutine,
	HalPowerDownRoutine,
	HalRestartRoutine,
	HalRebootRoutine,
	HalInteractiveModeRoutine,
	HalMaximumRoutine
} FIRMWARE_REENTRY, * PFIRMWARE_REENTRY;

typedef struct _OBJECT_TYPE {
	ERESOURCE Mutex;
	LIST_ENTRY TypeList;
	UNICODE_STRING Name;
	PVOID DefaultObject;
	ULONG Index;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	OBJECT_TYPE_INITIALIZER TypeInfo;
#ifdef POOL_TAGGING
	ULONG Key;
#endif
} OBJECT_TYPE, * POBJECT_TYPE;

typedef struct _OBJECT_HEADER {
	LONG PointerCount;
	union {
		LONG HandleCount;
		PSINGLE_LIST_ENTRY SEntry;
	};
	POBJECT_TYPE Type;
	UCHAR NameInfoOffset;
	UCHAR HandleInfoOffset;
	UCHAR QuotaInfoOffset;
	UCHAR Flags;
	union
	{
		PVOID ObjectCreateInfo;
		PVOID QuotaBlockCharged;
	};
	PSECURITY_DESCRIPTOR SecurityDescriptor;
	QUAD Body;
} OBJECT_HEADER, * POBJECT_HEADER;

typedef enum HardErrorResponseType {
	ResponseTypeAbortRetryIgnore,
	ResponseTypeOK,
	ResponseTypeOKCancel,
	ResponseTypeRetryCancel,
	ResponseTypeYesNo,
	ResponseTypeYesNoCancel,
	ResponseTypeShutdownSystem,
	ResponseTypeTrayNotify,
	ResponseTypeCancelTryAgainContinue
} HardErrorResponseType;

typedef enum HardErrorResponse {
	ResponseReturnToCaller,
	ResponseNotHandled,
	ResponseAbort,
	ResponseCancel,
	ResponseIgnore,
	ResponseNo,
	ResponseOk,
	ResponseRetry,
	ResponseYes
} HardErrorResponse;

typedef enum HardErrorResponseButton {
	ResponseButtonOK,
	ResponseButtonOKCancel,
	ResponseButtonAbortRetryIgnore,
	ResponseButtonYesNoCancel,
	ResponseButtonYesNo,
	ResponseButtonRetryCancel,
	ResponseButtonCancelTryAgainContinue
} HardErrorResponseButton;

typedef enum HardErrorResponseDefaultButton {
	DefaultButton1 = 0,
	DefaultButton2 = 0x100,
	DefaultButton3 = 0x200
} HardErrorResponseDefaultButton;

typedef enum HardErrorResponseIcon {
	IconAsterisk = 0x40,
	IconError = 0x10,
	IconExclamation = 0x30,
	IconHand = 0x10,
	IconInformation = 0x40,
	IconNone = 0,
	IconQuestion = 0x20,
	IconStop = 0x10,
	IconWarning = 0x30,
	IconUserIcon = 0x80
} HardErrorResponseIcon;

typedef enum HardErrorResponseOptions {
	ResponseOptionNone = 0,
	ResponseOptionDefaultDesktopOnly = 0x20000,
	ResponseOptionHelp = 0x4000,
	ResponseOptionRightAlign = 0x80000,
	ResponseOptionRightToLeftReading = 0x100000,
	ResponseOptionTopMost = 0x40000,
	ResponseOptionServiceNotification = 0x00200000,
	ResponseOptionServiceNotificationNT3X = 0x00040000,
	ResponseOptionSetForeground = 0x10000,
	ResponseOptionSystemModal = 0x1000,
	ResponseOptionTaskModal = 0x2000,
	ResponseOptionNoFocus = 0x00008000
} HardErrorResponseOptions;

typedef LONG KPRIORITY;

#ifdef __cplusplus
EXTERN_C_START
#endif // __cplusplus

NTSTATUS
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS,
	IN PVOID, IN ULONG,
	IN PULONG
);

NTSYSAPI
NTSTATUS NTAPI ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID* Object
);

VOID
HalReturnToFirmware(
	IN FIRMWARE_REENTRY Routine
);

NTKERNELAPI
NTSTATUS
ZwAssignProcessToJobObject(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
);

NTKERNELAPI
NTSTATUS
ZwTerminateJobObject(
	IN HANDLE JobHandle,
	IN NTSTATUS ExitStatus
);

NTKERNELAPI
NTSTATUS
ZwCreateJobObject(
	OUT PHANDLE  JobHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes
);

NTSTATUS
ObCreateObject(
	__in KPROCESSOR_MODE ProbeMode,            // 决定是否要验证参数
	__in POBJECT_TYPE ObjectType,              // 对象类型指针
	__in POBJECT_ATTRIBUTES ObjectAttributes,  // 对象的属性, 最终会转化成ObAllocateObject需要的OBJECT_CREATE_INFORMATION结构
	__in KPROCESSOR_MODE OwnershipMode,        // 内核对象?用户对象? 同上
	__inout_opt PVOID ParseContext,            // 这参数没用
	__in ULONG ObjectBodySize,                 // 对象体大小
	__in ULONG PagedPoolCharge,                // ...
	__in ULONG NonPagedPoolCharge,             // ...
	__out PVOID* Object                        // 接收对象体的指针
);

NTSTATUS
SeCreateAccessState(
	__out PACCESS_STATE AccessState,
	__out PAUX_ACCESS_DATA AuxData,
	__in ACCESS_MASK DesiredAccess,
	__in_opt PGENERIC_MAPPING GenericMapping
);

NTSTATUS
NTAPI
ExRaiseHardError(
	IN NTSTATUS ErrorStatus,
	IN ULONG NumberOfParameters,
	IN ULONG UnicodeStringParameterMask,
	IN PVOID Parameters,
	IN ULONG ResponseOption,
	OUT PULONG Response
);

extern POBJECT_TYPE* IoDriverObjectType;

extern PULONG InitSafeBootMode;

#ifdef __cplusplus
EXTERN_C_END
#endif // __cplusplus