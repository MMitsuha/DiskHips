#include "DiskHandler.hpp"

DiskHandler::DiskHandler(
	VOID
)
{
	ClearAll();
}

DiskHandler::DiskHandler(
	IN PUNICODE_STRING puniDiskName
)
{
	ClearAll();
	OpenDisk(puniDiskName);
}

DiskHandler::DiskHandler(
	IN DiskHandler&& Disk
)
{
	hDisk = Disk.hDisk;
	Disk.hDisk = NULL;
}

DiskHandler&
DiskHandler::operator=(
	IN DiskHandler&& Disk
	)
{
	if (this != &Disk)
	{
		CloseDisk();
		hDisk = Disk.hDisk;
		Disk.hDisk = NULL;
	}
	return *this;
}

NTSTATUS
DiskHandler::OpenDisk(
	IN PUNICODE_STRING puniDiskName
)
{
	if (!puniDiskName)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		IO_STATUS_BLOCK IoStatusBlock = { 0 };
	OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(puniDiskName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE);
	ntStatus = ZwOpenFile(&hDisk, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(ntStatus))
		CloseDisk();

	TRY_END(ntStatus)
}

NTSTATUS
DiskHandler::ReadDisk(
	IN PVOID SectorBuffer,
	IN ULONG SectorBufferSize,
	IN UINT64 SectorOffset
)
{
	if (!hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwReadFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, SectorBuffer, (UINT64)SectorBufferSize * 512, &Offset, NULL);
	if (!NT_SUCCESS(ntStatus))
		RtlZeroMemory(SectorBuffer, (UINT64)SectorBufferSize * 512);

	TRY_END(ntStatus)
}

NTSTATUS
DiskHandler::WriteDisk(
	IN PVOID SectorBuffer,
	IN ULONG SectorBufferSize,
	IN UINT64 SectorOffset
)
{
	if (!hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwWriteFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, SectorBuffer, (UINT64)SectorBufferSize * 512, &Offset, NULL);
	if (!NT_SUCCESS(ntStatus))
		RtlZeroMemory(SectorBuffer, (UINT64)SectorBufferSize * 512);

	TRY_END(ntStatus)
}

DiskHandler::~DiskHandler(
	VOID
)
{
	CloseDisk();
}

VOID
DiskHandler::ClearAll(
	VOID
)
{
	hDisk = NULL;
}

VOID
DiskHandler::CloseDisk(
	VOID
)
{
	if (!hDisk)
		return;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		ntStatus = ZwClose(hDisk);
	if (NT_SUCCESS(ntStatus))
		hDisk = NULL;

	TRY_END_NOSTATUS
}