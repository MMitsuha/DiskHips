#pragma once
#include "Includes.hpp"
#include "Undisclosed.hpp"
#include "UsingCPP.hpp"

typedef NTSTATUS(*IRP_MJ_SERIES)
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
	);

class FSDHook
{
public:
	FSDHook(
		VOID
	);

	FSDHook(
		IN PUNICODE_STRING puniDriverName,
		IN SHORT Subscript,
		IN IRP_MJ_SERIES CurFunc
	);

	NTSTATUS
		StartHook(
			IN PUNICODE_STRING puniDriverName,
			IN SHORT Subscript,
			IN IRP_MJ_SERIES CurFunc
		);

	NTSTATUS
		CallOriFunc(
			IN PDEVICE_OBJECT pDeviceObject,
			IN PIRP pIrp
		);

	virtual
		NTSTATUS
		StopHook(
			VOID
		);

	virtual
		NTSTATUS
		RestartHook(
			VOID
		);

	virtual
		~FSDHook(
			VOID
		) {};

private:
	VOID
		ClearAll(
			VOID
		);

	PDRIVER_OBJECT pDriverObject;
	IRP_MJ_SERIES OriFunc;
	IRP_MJ_SERIES CurerntFunc;
	SHORT Subscript;
};