#include "DeviceHandler.hpp"

DeviceHandler::DeviceHandler(
	OUT PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING puniDeviceName,
	IN PUNICODE_STRING puniSymbolName,
	IN ULONG ulDeviceExternsionSize,
	IN BOOLEAN Exclusive
)
{
	TRY_START

		CreateDevice(pDriverObject, puniDeviceName, puniSymbolName, ulDeviceExternsionSize, Exclusive);

	TRY_END_NOSTATUS
}

NTSTATUS
DeviceHandler::CreateDevice(
	OUT PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING puniDeviceName,
	IN PUNICODE_STRING puniSymbolName,
	IN ULONG ulDeviceExternsionSize,
	IN BOOLEAN Exclusive
)
{
	if (!pDriverObject || pDeviceObject)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		do
		{
			pDriverObject->Flags |= DO_BUFFERED_IO;

			ntStatus = IoCreateDevice(pDriverObject, ulDeviceExternsionSize, puniDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, Exclusive, &pDeviceObject);
			if (!NT_SUCCESS(ntStatus))
			{
				PrintErr("�����豸����ʧ��! Errorcode:%X\n", ntStatus);
				break;
			}

			uniSymbolName.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, puniSymbolName->Length, 'NS');
			if (!uniSymbolName.Buffer)
			{
				ntStatus = STATUS_MEMORY_NOT_ALLOCATED;
				PrintErr("��ȡ�ڴ�ʧ��! Errorcode:%X\n", ntStatus);
				break;
			}
			RtlCopyMemory(uniSymbolName.Buffer, puniSymbolName->Buffer, puniSymbolName->Length);
			uniSymbolName.MaximumLength = uniSymbolName.Length = puniSymbolName->Length;

			if (!ulDeviceExternsionSize && pDeviceObject->DeviceExtension)
				RtlZeroMemory(pDeviceObject->DeviceExtension, ulDeviceExternsionSize);

			ntStatus = IoCreateSymbolicLink(puniSymbolName, puniDeviceName);
			if (!NT_SUCCESS(ntStatus))
			{
				PrintErr("������������ʧ��! Errorcode:%X\n", ntStatus);
				break;
			}
			bSymbolCreated = TRUE;
		} while (FALSE);

		if (!NT_SUCCESS(ntStatus))
			DeleteDevice();

		TRY_END(ntStatus)
}

NTSTATUS
DeviceHandler::DeleteDevice(
	VOID
)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		if (pDeviceObject)
		{
			IoDeleteDevice(pDeviceObject);
			pDeviceObject = NULL;
		}

	if (bSymbolCreated)
	{
		ntStatus = IoDeleteSymbolicLink(&uniSymbolName);
		if (!NT_SUCCESS(ntStatus))
			BugCheck(WK_UNABLE_TO_DELETE_SYMBOLIC_LINK, ntStatus, 0, 0, 0);
		else
			bSymbolCreated = FALSE;
	}

	if (uniSymbolName.Buffer)
	{
		ExFreePoolWithTag(uniSymbolName.Buffer, 'NS');
		RtlZeroMemory(&uniSymbolName, sizeof(uniSymbolName));
	}

	TRY_END(ntStatus)
}

DeviceHandler::DeviceHandler(
	IN DeviceHandler&& Device
)
{
	pDeviceObject = Device.pDeviceObject;
	Device.pDeviceObject = NULL;
	uniSymbolName = Device.uniSymbolName;
	RtlZeroMemory(&Device.uniSymbolName, sizeof(Device.uniSymbolName));
	bSymbolCreated = Device.bSymbolCreated;
	Device.bSymbolCreated = FALSE;
}

DeviceHandler&
DeviceHandler::operator=(
	IN DeviceHandler&& Device
	)
{
	if (this != &Device)
	{
		DeleteDevice();
		pDeviceObject = Device.pDeviceObject;
		Device.pDeviceObject = NULL;
		uniSymbolName = Device.uniSymbolName;
		RtlZeroMemory(&Device.uniSymbolName, sizeof(Device.uniSymbolName));
		bSymbolCreated = Device.bSymbolCreated;
		Device.bSymbolCreated = FALSE;
	}
	return *this;
}

PDEVICE_OBJECT
DeviceHandler::GetDeviceObject(
	VOID
)
{
	return pDeviceObject;
}

VOID
DeviceHandler::WriteDeviceExternsion(
	IN PVOID Data,
	IN ULONG Size
)
{
	TRY_START

		RtlCopyMemory(pDeviceObject->DeviceExtension, Data, Size);

	TRY_END_NOSTATUS
}

template<typename T>
inline
VOID
DeviceHandler::WriteDeviceExternsion(
	IN T& Data
)
{
	TRY_START

		RtlCopyMemory(pDeviceObject->DeviceExtension, &Data, sizeof(T));

	TRY_END_NOSTATUS
}