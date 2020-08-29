#pragma once
#include <windows.h>

class CDiskHander
{
public:
	CDiskHander(
		VOID
	);

	CDiskHander(
		IN PWCHAR wstrDiskName
	);

	CDiskHander(
		IN CONST CDiskHander&
	) = delete;

	ULONG
		Attach(
			IN CONST PWCHAR wstrDiskName
		);

	ULONG
		Write(
			IN UINT64 StartSector,
			IN CONST PBYTE  Buffer,
			IN ULONG SectorsCount
		);

	ULONG
		Read(
			IN UINT64 StartSector,
			OUT PBYTE Buffer,
			IN ULONG SectorsCount
		);

	ULONG
		ForMatSector(
			IN UINT64 SectorNum
		);

	HANDLE
		GetHandle(
			VOID
		);

	BOOL
		Detach(
			VOID
		);

	virtual ~CDiskHander(
		VOID
	);

private:
	HANDLE Handle;
};

typedef CDiskHander* PCDiskHander;