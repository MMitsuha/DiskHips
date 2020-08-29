#pragma once
#include "DeviceHandler.hpp"

template <typename T>
class MessengerDevice
{
public:
	MessengerDevice(
		VOID
	) {};

	MessengerDevice(
		OUT PDRIVER_OBJECT pDriverObject,
		IN PUNICODE_STRING puniEventName,
		IN PUNICODE_STRING puniDeviceName = NULL,
		IN PUNICODE_STRING puniSymbolName = NULL,
		IN ULONG ulDeviceExternsionSize = 0,
		IN BOOLEAN Exclusive = FALSE
	);

	NTSTATUS
		CreateMessenger(
			OUT PDRIVER_OBJECT pDriverObject,
			IN PUNICODE_STRING puniEventName,
			IN PUNICODE_STRING puniDeviceName = NULL,
			IN PUNICODE_STRING puniSymbolName = NULL,
			IN ULONG ulDeviceExternsionSize = 0,
			IN BOOLEAN Exclusive = FALSE
		);

	MessengerDevice(
		IN CONST MessengerDevice&
	) = delete;

	VOID
		ResetAllStatus(
			VOID
		);

	BOOLEAN
		ResetSelfLock(
			VOID
		);

	BOOLEAN
		ResetKernelToUser(
			VOID
		);

	BOOLEAN
		ResetUserToKernel(
			VOID
		);

	BOOLEAN
		SetSelfLock(
			VOID
		);

	BOOLEAN
		SetKernelToUser(
			VOID
		);

	BOOLEAN
		SetUserToKernel(
			VOID
		);

	VOID
		WriteData(
			IN T* Data
		);

	VOID
		ReadData(
			OUT T* Data
		);

	T
		ReadData(
			VOID
		);

	VOID
		WriteCreateStatus(
			IN BOOLEAN bStatus
		);

	BOOLEAN
		ReadCreateStatus(
			VOID
		);

	VOID
		WriteChoose(
			IN BOOLEAN bChoose
		);

	BOOLEAN
		ReadChoose(
			VOID
		);

	NTSTATUS
		WaitForSelfLock(
			VOID
		);

	NTSTATUS
		WaitForKernelToUser(
			VOID
		);

	NTSTATUS
		WaitForUserToKernel(
			VOID
		);

	NTSTATUS
		DeleteMessager(
			VOID
		);

	virtual ~MessengerDevice(
		VOID
	) {};

private:
	T UserBuffer;

	DeviceHandler DeviceObject;
	HANDLE hKernelToUserEventHandle = NULL;
	PKEVENT pkKernelToUserEvent = NULL;

	KEVENT kKernelEvent = { 0 };
	KEVENT kUserToKernelEvent = { 0 };

	BOOLEAN bIsCreated = FALSE;

	BOOLEAN bIsRefuse = FALSE;

	//KSPIN_LOCK SpinLock = { 0 };
};

#include "MessengerDevice.hpp"

template<typename T>
MessengerDevice<T>::MessengerDevice(
	OUT PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING puniEventName,
	IN PUNICODE_STRING puniDeviceName,
	IN PUNICODE_STRING puniSymbolName,
	IN ULONG ulDeviceExternsionSize,
	IN BOOLEAN Exclusive
)
{
	TRY_START

		CreateMessenger(pDriverObject, puniEventName, puniDeviceName, puniSymbolName, ulDeviceExternsionSize, Exclusive);

	TRY_END_NOSTATUS
}

template<typename T>
NTSTATUS
MessengerDevice<T>::CreateMessenger(
	OUT PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING puniEventName,
	IN PUNICODE_STRING puniDeviceName,
	IN PUNICODE_STRING puniSymbolName,
	IN ULONG ulDeviceExternsionSize,
	IN BOOLEAN Exclusive
)
{
	if (!pDriverObject || !puniEventName)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		RtlZeroMemory(&UserBuffer, sizeof(T));

	ntStatus = DeviceObject.CreateDevice(pDriverObject, puniDeviceName, puniSymbolName, ulDeviceExternsionSize, Exclusive);
	if (!NT_SUCCESS(ntStatus))
	{
		PrintErr("创建设备失败! Errorcode:%X\n", ntStatus);
		DeviceObject.DeleteDevice();
		return ntStatus;
	}

	pkKernelToUserEvent = IoCreateSynchronizationEvent(puniEventName, &hKernelToUserEventHandle);
	if (!pkKernelToUserEvent)
	{
		PrintErr("创建全局事件失败!\n");
		ntStatus = STATUS_UNSUCCESSFUL;
	}
	else
		ntStatus = STATUS_SUCCESS;
	KeClearEvent(pkKernelToUserEvent);

	KeInitializeEvent(&kKernelEvent, SynchronizationEvent, TRUE);
	KeInitializeEvent(&kUserToKernelEvent, SynchronizationEvent, FALSE);

	//KeInitializeSpinLock(&SpinLock);

	TRY_END(ntStatus)
}

template<typename T>
VOID
MessengerDevice<T>::ResetAllStatus(
	VOID
)
{
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	SetSelfLock();
	ResetKernelToUser();
	ResetUserToKernel();
	//ExReleaseSpinLock(&SpinLock, IRQL);
	return;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::ResetSelfLock(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeResetEvent(&kKernelEvent);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(bRet);
	return bRet;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::ResetKernelToUser(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeResetEvent(pkKernelToUserEvent);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(bRet);
	return bRet;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::ResetUserToKernel(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeResetEvent(&kUserToKernelEvent);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(bRet);
	return bRet;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::SetSelfLock(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeSetEvent(&kKernelEvent, 0, FALSE);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(!bRet);
	return bRet;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::SetKernelToUser(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeSetEvent(pkKernelToUserEvent, 0, FALSE);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(!bRet);
	return bRet;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::SetUserToKernel(
	VOID
)
{
	BOOLEAN bRet = TRUE;
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bRet = (BOOLEAN)KeSetEvent(&kUserToKernelEvent, 0, FALSE);
	//ExReleaseSpinLock(&SpinLock, IRQL);
	ASSERT(!bRet);
	return bRet;
}

template<typename T>
VOID
MessengerDevice<T>::WriteData(
	IN T* Data
)
{
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	RtlCopyMemory(&UserBuffer, Data, sizeof(T));
	//ExReleaseSpinLock(&SpinLock, IRQL);
	return;
}

template<typename T>
VOID
MessengerDevice<T>::ReadData(
	OUT T* Data
)
{
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	RtlCopyMemory(Data, &UserBuffer, sizeof(T));
	//ExReleaseSpinLock(&SpinLock, IRQL);
	return;
}

template<typename T>
T
MessengerDevice<T>::ReadData(
	VOID
)
{
	return UserBuffer;
}

template<typename T>
VOID
MessengerDevice<T>::WriteCreateStatus(
	IN BOOLEAN bStatus
)
{
	bIsCreated = bStatus;
	return;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::ReadCreateStatus(
	VOID
)
{
	return bIsCreated;
}

template<typename T>
VOID
MessengerDevice<T>::WriteChoose(
	IN BOOLEAN bChoose
)
{
	//KIRQL IRQL = 0;
	//ExAcquireSpinLock(&SpinLock, &IRQL);
	bIsRefuse = bChoose;
	//ExReleaseSpinLock(&SpinLock, IRQL);
	return;
}

template<typename T>
BOOLEAN
MessengerDevice<T>::ReadChoose(
	VOID
)
{
	return bIsRefuse;
}

template<typename T>
NTSTATUS
MessengerDevice<T>::WaitForSelfLock(
	VOID
)
{
	return KeWaitForSingleObject(&kKernelEvent, Executive, KernelMode, FALSE, NULL);
}

template<typename T>
NTSTATUS
MessengerDevice<T>::WaitForKernelToUser(
	VOID
)
{
	return KeWaitForSingleObject(pkKernelToUserEvent, Executive, KernelMode, FALSE, NULL);
}

template<typename T>
NTSTATUS
MessengerDevice<T>::WaitForUserToKernel(
	VOID
)
{
	return KeWaitForSingleObject(&kUserToKernelEvent, Executive, KernelMode, FALSE, NULL);
}

template<typename T>
NTSTATUS
MessengerDevice<T>::DeleteMessager(
	VOID
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		ntStatus = DeviceObject.DeleteDevice();
	if (!NT_SUCCESS(ntStatus))
		PrintErr("删除设备失败! Errorcode:%X\n", ntStatus);

	ResetSelfLock();
	ResetKernelToUser();
	ResetUserToKernel();
	ntStatus = ZwClose(hKernelToUserEventHandle);
	if (!NT_SUCCESS(ntStatus))
		PrintErr("删除事件失败! Errorcode:%X\n", ntStatus);
	hKernelToUserEventHandle = NULL;
	RtlZeroMemory(&kKernelEvent, sizeof(KEVENT));
	RtlZeroMemory(&kUserToKernelEvent, sizeof(KEVENT));
	RtlZeroMemory(&UserBuffer, sizeof(T));
	bIsCreated = FALSE;
	bIsRefuse = FALSE;

	TRY_END(ntStatus)
}