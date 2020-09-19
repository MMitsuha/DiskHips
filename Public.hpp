#pragma once
#include <windef.h>

#define USER_MBRHIPS_LINK_NAME L"\\\\.\\DHLink"
#define USER_MBRHIPS_EVENT_NAME L"Global\\DHEvent"

#ifdef CTL_CODE
#undef CTL_CODE
#endif
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define METHOD_BUFFERED                 0
#define FILE_DEVICE_UNKNOWN             0x00000022
#define MAKECODE(Function,Access) (CTL_CODE(FILE_DEVICE_UNKNOWN,Function,METHOD_BUFFERED,Access))

#define ENABLE_DISKHIPS_MONITOR MAKECODE(0x1C02,FILE_ANY_ACCESS)
#define DISABLE_DISKHIPS_MONITOR MAKECODE(0x1C03,FILE_ANY_ACCESS)

#define READ_DISKHIPS_DATA MAKECODE(0x1C00,FILE_READ_ACCESS)
#define WRITE_DISKHIPS_DATA MAKECODE(0x1C01,FILE_WRITE_ACCESS)

#define ADD_DENY_SECTOR MAKECODE(0x1C04,FILE_WRITE_ACCESS)
#define DEL_DENY_SECTOR MAKECODE(0x1C05,FILE_WRITE_ACCESS)

#define TEST MAKECODE(0x1C06,FILE_ANY_ACCESS)

typedef struct _USER_CHOOSE
{
	PVOID pIrp;

	BOOLEAN bIsDenied;
}USER_CHOOSE, * PUSER_CHOOSE;

typedef struct _USER_BUFFER
{
	WCHAR wstrNTProcessPath[MAX_PATH];
	WCHAR wstrProcessName[MAX_PATH];
	HANDLE hProcID;
	UINT64 Offset;
	ULONG Length;
	PVOID pIrp;

	BOOLEAN bIsWrite;
}USER_BUFFER, * PUSER_BUFFER;