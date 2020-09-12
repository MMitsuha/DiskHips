#pragma once
#include "Includes.hpp"
#include "UsingCPP.hpp"

class DiskHandler
{
public:
	DiskHandler(
		VOID
	);

	DiskHandler(
		IN PUNICODE_STRING puniDiskName
	);

	DiskHandler(
		IN CONST DiskHandler&
	) = delete;

	DiskHandler(
		IN DiskHandler&& Disk
	);

	DiskHandler&
		operator=(
			IN CONST DiskHandler&
			) = delete;

	DiskHandler&
		operator=(
			IN DiskHandler&& Disk
			);

	NTSTATUS
		OpenDisk(
			IN PUNICODE_STRING puniDiskName
		);

	NTSTATUS
		ReadDisk(
			IN PVOID SectorBuffer,
			IN ULONG SectorBufferSize,
			IN UINT64 SectorOffset
		);

	NTSTATUS
		WriteDisk(
			IN PVOID SectorBuffer,
			IN ULONG SectorBufferSize,
			IN UINT64 SectorOffset
		);

	template <typename T>
	NTSTATUS
		WriteDisk(
			IN CONST T& SectorBuffer,
			IN UINT64 SectorOffset
		);

	template <typename T, ULONG _Size>
	friend
		NTSTATUS
		WriteDisk(
			IN CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset,
			IN CONST DiskHandler& Disk
		);

	template <typename T, ULONG _Size>
	NTSTATUS
		ArrayWriteDisk(
			IN CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset
		);

	template <typename T, ULONG _Size>
	friend
		NTSTATUS
		WriteDisk(
			IN CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset,
			IN CONST DiskHandler* CONST Disk
		);

	template <typename T>
	NTSTATUS
		ReadDisk(
			OUT T& SectorBuffer,
			IN UINT64 SectorOffset
		);

	template <typename T, ULONG _Size>
	friend
		NTSTATUS
		ReadDisk(
			OUT CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset,
			IN CONST DiskHandler& Disk
		);

	template <typename T, ULONG _Size>
	NTSTATUS
		ArrayReadDisk(
			OUT CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset
		);

	template <typename T, ULONG _Size>
	friend
		NTSTATUS
		ReadDisk(
			OUT CONST T(&Buffer)[_Size],
			IN UINT64 SectorOffset,
			IN CONST DiskHandler* CONST Disk
		);

	virtual
		VOID
		CloseDisk(
			VOID
		);

	virtual
		~DiskHandler(
			VOID
		);

private:
	VOID
		ClearAll(
			VOID
		);

	HANDLE hDisk;
};

template<typename T>
NTSTATUS
DiskHandler::WriteDisk(
	IN CONST T& Buffer,
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
	ntStatus = ZwWriteFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, &Buffer, sizeof(Buffer), &Offset, NULL);

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
DiskHandler::ArrayWriteDisk(
	IN CONST T(&Buffer)[_Size],
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
	ntStatus = ZwWriteFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
DiskHandler::ArrayReadDisk(
	OUT CONST T(&Buffer)[_Size],
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
	ntStatus = ZwReadFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}

template<typename T>
NTSTATUS
DiskHandler::ReadDisk(
	OUT T& Buffer,
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
	ntStatus = ZwReadFile(hDisk, NULL, NULL, NULL, &IoStatusBlock, &Buffer, sizeof(Buffer), &Offset, NULL);
	if (!NT_SUCCESS(ntStatus))
		RtlZeroMemory(SectorBuffer, sizeof(Buffer));

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
WriteDisk(
	IN CONST T(&Buffer)[_Size],
	IN UINT64 SectorOffset,
	IN CONST DiskHandler& Disk
)
{
	if (!Disk.hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwWriteFile(Disk.hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
ReadDisk(
	OUT CONST T(&Buffer)[_Size],
	IN UINT64 SectorOffset,
	IN CONST DiskHandler& Disk
)
{
	if (!Disk.hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwReadFile(Disk.hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
WriteDisk(
	IN CONST T(&Buffer)[_Size],
	IN UINT64 SectorOffset,
	IN CONST DiskHandler* CONST Disk
)
{
	if (!Disk->hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwWriteFile(Disk->hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}

template <typename T, ULONG _Size>
NTSTATUS
ReadDisk(
	OUT CONST T(&Buffer)[_Size],
	IN UINT64 SectorOffset,
	IN CONST DiskHandler* CONST Disk
)
{
	if (!Disk->hDisk)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

	TRY_START

		LARGE_INTEGER Offset = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	Offset.QuadPart = SectorOffset * 512;
	ntStatus = ZwReadFile(Disk->hDisk, NULL, NULL, NULL, &IoStatusBlock, (PVOID)Buffer, sizeof(T) * _Size, &Offset, NULL);

	TRY_END(ntStatus)
}