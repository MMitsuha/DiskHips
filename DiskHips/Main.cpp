#include "Includes.hpp"
#include "MessengerDevice.hpp"
#include "FSDHook.hpp"
#include "Undisclosed.hpp"
#include "DiskHandler.hpp"
#include "../Public.hpp"
#include <vector>
#include <algorithm>

#define KERNEL_DISKHIPS_DEVICE_NAME L"\\Device\\DHDevice"
#define KERNEL_DISKHIPS_LINK_NAME L"\\??\\DHLink"
#define KERNEL_DISKHIPS_EVENT_NAME L"\\BaseNamedObjects\\DHEvent"

typedef NTSTATUS(*IRP_MJ_SERIES)
(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP pIrp
	);

typedef struct _IRP_INFORMATION
{
	PDEVICE_OBJECT pDeviceObject;
	PIRP pIrp;
} IRP_INFORMATION, * PIRP_INFORMATION;

EXTERN_C_START

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
);

NTSYSAPI
NTSTATUS
NTAPI
ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID* Object
);

extern POBJECT_TYPE* IoDriverObjectType;

EXTERN_C_END

typedef struct _OBJECT_CREATE_INFORMATION
{
	ULONG Attributes;
	HANDLE RootDirectory;
	PVOID ParseContext;
	KPROCESSOR_MODE ProbeMode;
	ULONG PagedPoolCharge;
	ULONG NonPagedPoolCharge;
	ULONG SecurityDescriptorCharge;
	PSECURITY_DESCRIPTOR SecurityDescriptor;
	PSECURITY_QUALITY_OF_SERVICE SecurityQos;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, * POBJECT_CREATE_INFORMATION;

#define NUMBER_HASH_BUCKETS 37
typedef struct _OBJECT_DIRECTORY
{
	struct _OBJECT_DIRECTORY_ENTRY* HashBuckets[NUMBER_HASH_BUCKETS];
	struct _OBJECT_DIRECTORY_ENTRY** LookupBucket;
	BOOLEAN LookupFound;
	USHORT SymbolicLinkUsageCount;
	struct _DEVICE_MAP* DeviceMap;
} OBJECT_DIRECTORY, * POBJECT_DIRECTORY;

typedef struct _OBJECT_HEADER_NAME_INFO
{
	POBJECT_DIRECTORY Directory;
	UNICODE_STRING Name;
	ULONG Reserved;
#if DBG
	ULONG Reserved2;
	LONG DbgDereferenceCount;
#endif
} OBJECT_HEADER_NAME_INFO, * POBJECT_HEADER_NAME_INFO;

#define OBJECT_HEADER_TO_NAME_INFO( oh ) ((POBJECT_HEADER_NAME_INFO) \
    ((oh)->NameInfoOffset == 0 ? NULL : ((PCHAR)(oh) - (oh)->NameInfoOffset)))

#define NSToS(s) s*1000000000

//--------------------------------------------

MessengerDevice<USER_BUFFER> g_DeviceObject;
//PDRIVER_OBJECT g_pDiskDriverObject = NULL;
//IRP_MJ_SERIES g_OriDiskFunc = NULL;
FSDHook g_DiskObject;
vector<UINT64> WarningSectors;
vector<IRP_INFORMATION> g_PendingIrp;

UNICODE_STRING g_uniDiskName = RTL_CONSTANT_STRING(L"\\Driver\\Disk");

VOID
CancelRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP pIrp
)
{
	TRY_START

		for (auto iter = g_PendingIrp.begin(); iter != g_PendingIrp.end(); ++iter)
			if ((*iter).pIrp == pIrp)
				g_PendingIrp.erase(iter);

	pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoReleaseCancelSpinLock(pIrp->CancelIrql);

	TRY_END_NOSTATUS
}

NTSTATUS
StartProtectDisk(
	OUT PDRIVER_OBJECT* OutDevice,
	OUT IRP_MJ_SERIES* OriFunc,
	IN IRP_MJ_SERIES CurrentFunc
)
{
	if (!OutDevice || !OriFunc || !CurrentFunc)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		UNICODE_STRING uniDiskName = RTL_CONSTANT_STRING(L"\\Driver\\Disk");
	/*
	ntStatus = ObReferenceObjectByName(&uniDiskName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)OutDevice);
	if (NT_SUCCESS(ntStatus))
		*OriFunc = (IRP_MJ_SERIES)InterlockedExchange64((PLONG64)(&((*OutDevice)->MajorFunction[IRP_MJ_WRITE])), (LONG64)CurrentFunc);
		*/
	ntStatus = g_DiskObject.StartHook(&uniDiskName, IRP_MJ_WRITE, CurrentFunc);

	TRY_END(ntStatus)
}

NTSTATUS
DefaultDispatch(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	TRY_START

		ntStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = ntStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	TRY_END(ntStatus)
}

NTSTATUS
CloseDispatch(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	TRY_START

		ntStatus = g_DiskObject.StopHook();
	if (!NT_SUCCESS(ntStatus))
		PrintErr("回滚操作失败! 错误码:%X\n", ntStatus);
	for (auto iter = g_PendingIrp.begin(); iter != g_PendingIrp.end(); ++iter)
		CancelRoutine((*iter).pDeviceObject, (*iter).pIrp);

	g_DeviceObject.ResetAllStatus();

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = ntStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	TRY_END(ntStatus)
}

VOID
DriverUnload(
	IN PDRIVER_OBJECT pDriverObject
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		ntStatus = g_DiskObject.StopHook();
	if (!NT_SUCCESS(ntStatus))
		PrintErr("回滚操作失败! 错误码:%X\n", ntStatus);

	ntStatus = g_DeviceObject.DeleteDevice();
	if (!NT_SUCCESS(ntStatus))
		PrintErr("删除设备失败! 错误码:%X\n", ntStatus);

	TRY_END_NOSTATUS
}

NTSTATUS
FilterFunction(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	__try {
		do
		{
			if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
				return g_DiskObject.CallOriFunc(pDeviceObject, pIrp);

			PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pIrp);
			if (!IrpSp)
			{
				PrintErr("[%S] 无效的参数:IrpSp!\n", __FUNCTIONW__);
				ntStatus = STATUS_INVALID_PARAMETER;
				break;
			}
			HANDLE hProc = PsGetCurrentProcessId();
			UINT64 Sector = (UINT64)IrpSp->Parameters.Write.ByteOffset.QuadPart / 512;
			BOOLEAN IsFound = FALSE;
			for (auto CurrentSector : WarningSectors)
				if (CurrentSector == Sector)
					IsFound = TRUE;

#if _DEBUG
			if ((hProc == (HANDLE)4 || hProc == (HANDLE)0) && !IsFound)
				return g_DiskObject.CallOriFunc(pDeviceObject, pIrp);
#endif // _DEBUG

			PEPROCESS pEProc = IoThreadToProcess(pIrp->Tail.Overlay.Thread);
			if (!pEProc)
			{
				PrintErr("[%S] 无效的参数:pEProc!\n", __FUNCTIONW__);
				ntStatus = STATUS_INVALID_PARAMETER;
				break;
			}
			PUNICODE_STRING puniProcImageName = NULL;
			ntStatus = SeLocateProcessImageName(pEProc, &puniProcImageName);
			if (!NT_SUCCESS(ntStatus) || !puniProcImageName)
			{
				PrintErr("[%S] SeLocateProcessImageName失败! 错误码:%X\n", __FUNCTIONW__, ntStatus);
				break;
			}

			if (hProc == (HANDLE)4 || hProc == (HANDLE)0)
			{
				g_DeviceObject.WaitForSelfLock();

				USER_BUFFER UserBuffer = { 0 };

				UserBuffer.hProcID = hProc;
				RtlZeroMemory(UserBuffer.wstrNTProcessPath, sizeof(UserBuffer.wstrNTProcessPath));

				RtlZeroMemory(UserBuffer.wstrProcessName, sizeof(UserBuffer.wstrProcessName));

				UserBuffer.Offset = IrpSp->Parameters.Write.ByteOffset.QuadPart;
				UserBuffer.Length = IrpSp->Parameters.Write.Length;
				UserBuffer.pIrp = pIrp;

				g_DeviceObject.WriteData(&UserBuffer);
				g_DeviceObject.SetKernelToUser();
			}
			else
			{
				BOOLEAN bIsFound = FALSE;
				//SIZE_T LastAllocated = 0;
				//PVOID pBuffer = NULL;
				while (!bIsFound)
				{
					DWORD dwNeedSize = 0;
					ntStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, NULL, 0, &dwNeedSize);
					if (ntStatus == STATUS_INFO_LENGTH_MISMATCH)
					{
						/*
						if (LastAllocated < dwNeedSize || !pBuffer)
						{
							PrintIfm("LastAllocated < dwNeedSize 分配内存! 大小:%X", dwNeedSize);
							if (pBuffer)
								ExFreePoolWithTag(pBuffer, 'WK');
							pBuffer = NULL;
							pBuffer = ExAllocatePoolWithTag(NonPagedPool, dwNeedSize, 'WK');
							if (!pBuffer)
								continue;
							LastAllocated = dwNeedSize;
						}
						else
							PrintIfm("LastAllocated >= dwNeedSize 未分配内存!");
						*/
						PVOID pBuffer = ExAllocatePoolWithTag(NonPagedPool, dwNeedSize, 'WK');
						PrintIfm("ZwQuerySystemInformation 分配内存! 大小:%X", dwNeedSize);

						if (pBuffer)
						{
							//LastAllocated = dwNeedSize;
							ntStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, pBuffer, dwNeedSize, NULL);
							if (NT_SUCCESS(ntStatus))
							{
								PSYSTEM_PROCESSES pSysProc = (PSYSTEM_PROCESSES)pBuffer;
								do
								{
									if (pSysProc->ProcessId == hProc)
									{
										g_DeviceObject.WaitForSelfLock();

										USER_BUFFER UserBuffer = { 0 };

										UserBuffer.hProcID = hProc;
										RtlCopyMemory(UserBuffer.wstrNTProcessPath, puniProcImageName->Buffer, puniProcImageName->Length);
										UserBuffer.wstrNTProcessPath[puniProcImageName->Length / sizeof(WCHAR)] = 0;

										RtlCopyMemory(UserBuffer.wstrProcessName, pSysProc->ProcessName.Buffer, pSysProc->ProcessName.Length);
										UserBuffer.wstrProcessName[pSysProc->ProcessName.Length / sizeof(WCHAR)] = 0;

										UserBuffer.Offset = IrpSp->Parameters.Write.ByteOffset.QuadPart;
										UserBuffer.Length = IrpSp->Parameters.Write.Length;
										UserBuffer.pIrp = pIrp;

										g_DeviceObject.WriteData(&UserBuffer);
										g_DeviceObject.SetKernelToUser();

										bIsFound = TRUE;

										break;
									}

									pSysProc = (PSYSTEM_PROCESSES)((ULONG_PTR)pSysProc + (ULONG_PTR)pSysProc->NextEntryDelta);
								} while (pSysProc->NextEntryDelta != 0);
							}
							ExFreePoolWithTag(pBuffer, 'WK');
							pBuffer = NULL;
						}
						else
						{
							ntStatus = STATUS_MEMORY_NOT_ALLOCATED;
							break;
						}
					}
					else
					{
						ntStatus = STATUS_UNSUCCESSFUL;
						break;
					}

					PrintIfm("[%S] 未找到目标进程,正在重试...\n", __FUNCTIONW__);
				}
				if (!NT_SUCCESS(ntStatus))
					break;
			}

			if (hProc == (HANDLE)4 || hProc == (HANDLE)0)
				PrintIfm("[%S] ProcPath:System ,Sector:%I64u ,ProcID:%I64u ,ProcAddr:%p\n",
					__FUNCTIONW__,
					Sector,
					(UINT64)hProc,
					pEProc
				);
			else
				PrintIfm("[%S] ProcPath:%wZ ,Sector:%I64u ,ProcID:%I64u ,ProcAddr:%p\n",
					__FUNCTIONW__,
					puniProcImageName,
					Sector,
					(UINT64)hProc,
					pEProc
				);

			if (IsFound)
			{
				IRP_INFORMATION IrpInformation = { pDeviceObject,pIrp };
				g_PendingIrp.push_back(IrpInformation);
				IoSetCancelRoutine(pIrp, CancelRoutine);
				IoMarkIrpPending(pIrp);
				return STATUS_PENDING;
			}

			return g_DiskObject.CallOriFunc(pDeviceObject, pIrp);
		} while (FALSE);
	}
	__except (1)
	{
		PrintErr("[%S] 未知错误! 错误码:%X\n", __FUNCTIONW__, GetExceptionCode());
		g_DeviceObject.ResetAllStatus();
	}
	pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_ACCESS_DENIED;
}

NTSTATUS
IoControlDispatch(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	TRY_START

		ULONG_PTR Size = 0;

	do
	{
		PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(pIrp);
		if (!StackLocation)
			break;
		PVOID SystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG InBufferLength = StackLocation->Parameters.DeviceIoControl.InputBufferLength;
		ULONG OutBufferLength = StackLocation->Parameters.DeviceIoControl.OutputBufferLength;
		ULONG ControlCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;

		switch (ControlCode)
		{
			case READ_DISKHIPS_DATA:
				if (OutBufferLength < sizeof(USER_BUFFER))
				{
					PrintErr("缓冲区过小!\n");
					ntStatus = STATUS_BUFFER_TOO_SMALL;
					break;
				}

				g_DeviceObject.ReadData((PUSER_BUFFER)SystemBuffer);

				g_DeviceObject.SetSelfLock();

				Size = sizeof(USER_BUFFER);
				ntStatus = STATUS_SUCCESS;
				break;

			case WRITE_DISKHIPS_DATA:
				if (InBufferLength < sizeof(USER_CHOOSE))
				{
					PrintErr("缓冲区异常!\n");
					ntStatus = STATUS_BUFFER_TOO_SMALL;
					break;
				}

				for (auto iter = g_PendingIrp.begin(); iter != g_PendingIrp.end(); ++iter)
					if ((*iter).pIrp == ((PUSER_CHOOSE)SystemBuffer)->pIrp)
					{
						if (((PUSER_CHOOSE)SystemBuffer)->bIsDenied)
						{
							(*iter).pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
							IoCompleteRequest((*iter).pIrp, IO_NO_INCREMENT);
							ntStatus = STATUS_SUCCESS;
						}
						else
							ntStatus = g_DiskObject.CallOriFunc((*iter).pDeviceObject, (*iter).pIrp);

						g_PendingIrp.erase(iter);

						Size = sizeof(USER_CHOOSE);
						break;
					}

				Size = 0;
				ntStatus = STATUS_INVALID_PARAMETER;
				break;

			case ENABLE_DISKHIPS_MONITOR:
				ntStatus = g_DiskObject.StartHook(&g_uniDiskName, IRP_MJ_WRITE, FilterFunction);
				break;

			case DISABLE_DISKHIPS_MONITOR:
				ntStatus = g_DiskObject.StopHook();
				for (auto iter = g_PendingIrp.begin(); iter != g_PendingIrp.end(); ++iter)
					CancelRoutine((*iter).pDeviceObject, (*iter).pIrp);
				break;

			case ADD_DENY_SECTOR:
				WarningSectors.push_back(*(PUINT64)SystemBuffer);

				ntStatus = STATUS_SUCCESS;
				Size = sizeof(UINT64);
				break;

			case DEL_DENY_SECTOR:
				for (auto iter = WarningSectors.begin(); iter != WarningSectors.end(); ++iter)
					if (*iter == *(PUINT64)SystemBuffer)
					{
						WarningSectors.erase(iter);
						ntStatus = STATUS_SUCCESS;
					}

				Size = sizeof(UINT64);
				break;

			case TEST:
			{
				DWORD dwNeedSize = 0;
				PBYTE pBuffer = NULL;
				PSYSTEM_PROCESSES pSP = NULL;
				ntStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, NULL, 0, &dwNeedSize);
				if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
					pBuffer = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, dwNeedSize, 'WK');
					ntStatus = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, (PVOID)pBuffer, dwNeedSize, NULL);
					if (NT_SUCCESS(ntStatus))
					{
						pSP = (PSYSTEM_PROCESSES)pBuffer;
						do {
							if ((ULONG_PTR)pSP->ProcessId == 0)
								KdPrint(("[0] System Idle Process"));
							else
								KdPrint(("[%I64u] %wZ", (UINT64)pSP->ProcessId, pSP->ProcessName));

							UNICODE_STRING uniProcessName1 = RTL_CONSTANT_STRING(L"HipsMain.exe");
							UNICODE_STRING uniProcessName2 = RTL_CONSTANT_STRING(L"HipsTray.exe");
							UNICODE_STRING uniProcessName3 = RTL_CONSTANT_STRING(L"HipsDaemon.exe");
							if (!RtlCompareUnicodeString(&pSP->ProcessName, &uniProcessName1, TRUE) || !RtlCompareUnicodeString(&pSP->ProcessName, &uniProcessName2, TRUE) || !RtlCompareUnicodeString(&pSP->ProcessName, &uniProcessName3, TRUE))
							{
								PEPROCESS EProcess = NULL;
								PsLookupProcessByProcessId(pSP->ProcessId, &EProcess);
								*(PVOID*)((ULONG_PTR)EProcess + 0x180) = (PVOID)(*(PUINT64)SystemBuffer);
								KdPrint(("%I64u", *(ULONG_PTR*)((ULONG_PTR)EProcess + 0x180)));
							}

							pSP = (PSYSTEM_PROCESSES)((UINT64)pSP + (UINT64)pSP->NextEntryDelta);
						} while (pSP->NextEntryDelta != 0);

						ExFreePool(pBuffer);
						pBuffer = NULL;
					}
				}
			}
			break;

			default:
				PrintErr("未知命令:%X\n", ControlCode);
				ntStatus = STATUS_INVALID_DEVICE_REQUEST;
				break;
		}
	} while (FALSE);

	pIrp->IoStatus.Information = Size;
	pIrp->IoStatus.Status = ntStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	TRY_END(ntStatus)
}

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	if (!pDriverObject)
		return STATUS_INVALID_PARAMETER;

	DbgPrint("[*] 驱动已加载,作者:caizhe666.\n");

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

	TRY_START

		do
		{
			UNICODE_STRING g_uniDiskHipsDeviceName = RTL_CONSTANT_STRING(KERNEL_DISKHIPS_DEVICE_NAME);
			UNICODE_STRING g_uniDiskHipsLinkName = RTL_CONSTANT_STRING(KERNEL_DISKHIPS_LINK_NAME);
			UNICODE_STRING g_uniDiskHipsEventName = RTL_CONSTANT_STRING(KERNEL_DISKHIPS_EVENT_NAME);
			ntStatus = g_DeviceObject.CreateDevice(pDriverObject, &g_uniDiskHipsEventName, &g_uniDiskHipsDeviceName, &g_uniDiskHipsLinkName, 0, TRUE);
			if (!NT_SUCCESS(ntStatus))
			{
				PrintErr("创建设备失败! 错误码:%X\n", ntStatus);
				break;
			}

			for (SHORT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
				pDriverObject->MajorFunction[i] = DefaultDispatch;
			pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControlDispatch;
			/*pDriverObject->MajorFunction[IRP_MJ_CLOSE] = */pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = CloseDispatch;
			pDriverObject->DriverUnload = DriverUnload;
		} while (FALSE);

		TRY_END(ntStatus)
}