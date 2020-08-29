#include "DiskHander.hpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDiskHander::CDiskHander(
	VOID
)
{
	Handle = INVALID_HANDLE_VALUE;
}

CDiskHander::CDiskHander(
	IN PWCHAR wstrDiskName
)
{
	Attach(wstrDiskName);
}

ULONG
CDiskHander::Attach(
	IN CONST PWCHAR wstrDiskName
)
{
	if (Handle != INVALID_HANDLE_VALUE)
		return ERROR_ALREADY_EXISTS;

	Handle = CreateFileW(wstrDiskName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	return GetLastError();
}

ULONG
CDiskHander::Write(
	IN UINT64 StartSector,
	IN CONST PBYTE Buffer,
	IN ULONG SectorsCount
)
{
	if (Handle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;

	LARGE_INTEGER liNewFilePointer = { 0 };

	liNewFilePointer.QuadPart = (UINT64)StartSector * 512;

	if (SetFilePointerEx(Handle, liNewFilePointer, NULL, FILE_BEGIN) == FALSE)
		return GetLastError();

	ULONG luReturn = 0;
	WriteFile(Handle, Buffer, SectorsCount * 512, &luReturn, NULL);

	return GetLastError();
}

ULONG
CDiskHander::Read(
	IN UINT64 StartSector,
	OUT PBYTE Buffer,
	IN ULONG SectorsCount
)
{
	if (Handle == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;

	LARGE_INTEGER liNewFilePointer = { 0 };

	liNewFilePointer.QuadPart = (UINT64)StartSector * 512;

	if (SetFilePointerEx(Handle, liNewFilePointer, NULL, FILE_BEGIN) == FALSE)
		return GetLastError();

	ULONG luReturn = 0;
	ReadFile(Handle, Buffer, SectorsCount * 512, &luReturn, NULL);

	return GetLastError();
}

ULONG
CDiskHander::ForMatSector(
	IN UINT64 SectorNum
)
{
	static CONST BYTE Clean[512] = { 0 };
	return Write(SectorNum, (CONST PBYTE)Clean, 1);
}

HANDLE
CDiskHander::GetHandle(
	VOID
)
{
	return Handle;
}

BOOL
CDiskHander::Detach(
	VOID
)
{
	if (Handle == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL bRet = CloseHandle(Handle);
	Handle = INVALID_HANDLE_VALUE;
	return bRet;
}

CDiskHander::~CDiskHander(
	VOID
)
{
	Detach();
}