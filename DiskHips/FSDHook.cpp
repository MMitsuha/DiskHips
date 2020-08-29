#include "FSDHook.hpp"

FSDHook::FSDHook(
	VOID
)
{
	TRY_START

		ClearAll();

	TRY_END_NOSTATUS
}

FSDHook::FSDHook(
	IN PUNICODE_STRING puniDriverName,
	IN SHORT Subscript,
	IN IRP_MJ_SERIES CurFunc
)
{
	TRY_START

		ClearAll();
	StartHook(puniDriverName, Subscript, CurFunc);

	TRY_END_NOSTATUS
}

NTSTATUS
FSDHook::StartHook(
	IN PUNICODE_STRING puniDriverName,
	IN SHORT FuncSubscript,
	IN IRP_MJ_SERIES CurFunc
)
{
	if (!puniDriverName || !CurFunc || FuncSubscript >= IRP_MJ_MAXIMUM_FUNCTION)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		ntStatus = ObReferenceObjectByName(puniDriverName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&pDriverObject);
	if (NT_SUCCESS(ntStatus))
	{
		OriFunc = (IRP_MJ_SERIES)InterlockedExchange64((PLONG64)(&pDriverObject->MajorFunction[FuncSubscript]), (LONG64)CurFunc);
		CurrentFunc = CurFunc;
		Subscript = FuncSubscript;
	}
	else
		ClearAll();

	TRY_END(ntStatus)
}

NTSTATUS
FSDHook::CallOriFunc(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
)
{
	return OriFunc(pDeviceObject, pIrp);
}

NTSTATUS
FSDHook::StopHook(
	VOID
)
{
	if (!pDriverObject || !OriFunc || Subscript >= IRP_MJ_MAXIMUM_FUNCTION)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		if (InterlockedExchange64((PLONG64)(&(pDriverObject->MajorFunction[Subscript])), (LONG64)OriFunc))
		{
			ClearAll();
			ntStatus = STATUS_SUCCESS;
		}

	TRY_END(ntStatus)
}

NTSTATUS
FSDHook::RestartHook(
	VOID
)
{
	if (!pDriverObject || !CurrentFunc || Subscript >= IRP_MJ_MAXIMUM_FUNCTION)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		if (InterlockedExchange64((PLONG64)(&(pDriverObject->MajorFunction[Subscript])), (LONG64)CurrentFunc))
			ntStatus = STATUS_SUCCESS;

	TRY_END(ntStatus)
}

VOID
FSDHook::ClearAll(
	VOID
)
{
	TRY_START

		if (pDriverObject)
		{
			ObDereferenceObject(pDriverObject);
			pDriverObject = NULL;
		}

	OriFunc = NULL;
	CurrentFunc = NULL;
	Subscript = IRP_MJ_MAXIMUM_FUNCTION;

	TRY_END_NOSTATUS
}