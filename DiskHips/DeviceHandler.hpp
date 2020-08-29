#pragma once
#include "Includes.hpp"
#include "UsingCPP.hpp"
#include "ErrorCode.hpp"

class DeviceHandler
{
public:
	DeviceHandler(
		VOID
	) {};

	//pDriverObject:驱动对象(必填),puniDeviceName:设备名,puniSymbolName:符号链接名,ulDeviceExternsionSize:附加拓展大小
	DeviceHandler(
		OUT PDRIVER_OBJECT pDriverObject,
		IN PUNICODE_STRING puniDeviceName = NULL,
		IN PUNICODE_STRING puniSymbolName = NULL,
		IN ULONG ulDeviceExternsionSize = 0,
		IN BOOLEAN Exclusive = FALSE
	);

	NTSTATUS
		CreateDevice(
			OUT PDRIVER_OBJECT pDriverObject,
			IN PUNICODE_STRING puniDeviceName = NULL,
			IN PUNICODE_STRING puniSymbolName = NULL,
			IN ULONG ulDeviceExternsionSize = 0,
			IN BOOLEAN Exclusive = FALSE
		);

	DeviceHandler(
		IN CONST DeviceHandler&
	) = delete;

	DeviceHandler(
		IN DeviceHandler&& Device
	);

	DeviceHandler&
		operator=(
			IN CONST DeviceHandler&
			) = delete;

	DeviceHandler&
		operator=(
			IN DeviceHandler&& Device
			);

	PDEVICE_OBJECT
		GetDeviceObject(
			VOID
		);

	VOID
		WriteDeviceExternsion(
			IN PVOID Data,
			IN ULONG Size
		);

	template<typename T>
	VOID
		WriteDeviceExternsion(
			IN T& Data
		);

	virtual
		NTSTATUS
		DeleteDevice(
			VOID
		);

	virtual ~DeviceHandler(
		VOID
	) {};

private:
	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING uniSymbolName = { 0 };
	BOOLEAN bSymbolCreated = FALSE;
};

typedef DeviceHandler* PDeviceHandler;