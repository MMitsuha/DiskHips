#include "Main.hpp"
#include "Functions.hpp"
#include <list>
#include <algorithm>

BOOLEAN
NtPathToDosPathW(
	_In_ PWSTR FullNtPath,
	_Out_ PWSTR FullDosPath
)
{
	if (!FullNtPath || !FullDosPath)
		return FALSE;

	WCHAR DosDevice[4] = { 0 };       //dos设备名最大长度为4
	WCHAR NtPath[64] = { 0 };       //nt设备名最大长度为64
	PWSTR RetStr = NULL;
	SIZE_T NtPathLen = 0;

	for (SHORT i = 65; i < 26 + 65; i++)
	{
		DosDevice[0] = i;
		DosDevice[1] = L':';
		if (QueryDosDeviceW(DosDevice, NtPath, 64))
		{
			if (NtPath)
			{
				NtPathLen = wcsnlen_s(NtPath, 64);
				if (!wcsncmp(NtPath, FullNtPath, NtPathLen))
				{
					wcscpy(FullDosPath, DosDevice);
					wcscat(FullDosPath, FullNtPath + NtPathLen);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

VOID SafeGetNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
	if (!lpSystemInfo)
		return;

	Func_GetSystemInfo funcGetNativeSystemInfo = (Func_GetSystemInfo)GetProcAddress(GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");

	if (funcGetNativeSystemInfo)
		funcGetNativeSystemInfo(lpSystemInfo);
	else
		GetSystemInfo(lpSystemInfo);
}

DWORD GetSystemBits()
{
	SYSTEM_INFO SystemInfo;
	SafeGetNativeSystemInfo(&SystemInfo);
	if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		return 0x64;
	else
		return 0x86;
}

INT
main(
	_In_ UINT argc,
	_In_ CHAR** argv
)
{
	WCHAR wstrDriverName[18] = { 0 };
	if (GetSystemBits() == 0x64)
		RtlCopyMemory(wstrDriverName, L".\\DiskHips_x64.sys", sizeof(L".\\DiskHips_x64.sys"));
	else
		RtlCopyMemory(wstrDriverName, L".\\DiskHips_x86.sys", sizeof(L".\\DiskHips_x86.sys"));
	if (!LoadNTDriver((PWCHAR)L"MH", wstrDriverName))
		cout << "启动驱动失败! 错误码:" << GetLastError() << endl;

	HANDLE hDriver = CreateFileW(USER_MBRHIPS_LINK_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDriver != INVALID_HANDLE_VALUE)
	{
		BOOL bRet = TRUE;
		DWORD ReturnedLength = 0;
		list<list<UINT64>> Object;
		list<UINT64> WarningSectors;
		cout << "已自动保护:" << flush;
		if (RecordAllSectors(Object))
		{
			for (auto CurrentList : Object)
				for (auto CurrentSector : CurrentList)
				{
					cout << CurrentSector << "," << flush;
					WarningSectors.push_back(CurrentSector);
				}
			cout << endl;

			BOOLEAN IsExpert = FALSE;
			cout << "是否进入专家模式(1 是/0 否):" << flush;
			cin >> IsExpert;
			if (IsExpert == 49)
			{
				cin.clear();
				cout << "输入你想保护的扇区(空格分隔,Ctrl+Z结束,例子:0 1 2 3^Z<Enter>):" << flush;
				UINT64 WarningSector = -1;
				while (cin >> WarningSector)
					WarningSectors.push_back(WarningSector);
				cout << endl;
				cin.clear();
				cin.get();
			}
			for (auto CurrentSector : WarningSectors)
			{
				BOOL bStatus = DeviceIoControl(hDriver, ADD_DENY_SECTOR, &CurrentSector, sizeof(CurrentSector), NULL, 0, &ReturnedLength, NULL);
				if (!bStatus)
				{
					bRet = FALSE;
					break;
				}
			}
			if (bRet)
			{
				bRet = DeviceIoControl(hDriver, ENABLE_DISKHIPS_MONITOR, NULL, 0, NULL, 0, &ReturnedLength, NULL);
				if (bRet)
				{
					HANDLE hEvent = OpenEventW(SYNCHRONIZE, FALSE, USER_MBRHIPS_EVENT_NAME);
					if (hEvent != INVALID_HANDLE_VALUE)
					{
						while (TRUE)
						{
							USER_BUFFER UserBuffer = { 0 };
							WaitForSingleObject(hEvent, INFINITE);
							bRet = DeviceIoControl(hDriver, READ_DISKHIPS_DATA, NULL, 0, &UserBuffer, sizeof(UserBuffer), &ReturnedLength, NULL);
							if (bRet)
							{
								cout << "------------------------------" << endl;
								if (UserBuffer.hProcID == (HANDLE)0 || UserBuffer.hProcID == (HANDLE)4)
									wcout << "进程名: " << L"System" << endl;
								else
									wcout << "进程名: " << UserBuffer.wstrProcessName << endl;
								cout << "PID: " << (UINT64)UserBuffer.hProcID << endl;
								if (UserBuffer.hProcID == (HANDLE)0 || UserBuffer.hProcID == (HANDLE)4)
									wcout << "进程路径: " << L"System" << endl;
								else
								{
									WCHAR DosPath[MAX_PATH + 4] = { 0 };
									NtPathToDosPathW(UserBuffer.wstrNTProcessPath, DosPath);
									wcout << "进程路径: " << DosPath << endl;
								}
								cout << "相对扇区偏移(字节): " << UserBuffer.Offset % 512 << endl;
								cout << "扇区(512字节): " << UserBuffer.Offset / 512 << endl;
								cout << "长度(字节): " << UserBuffer.Length << endl;

								BOOLEAN IsRebused = 49;
								for (auto CurrentSector : WarningSectors)
								{
									if (CurrentSector == UserBuffer.Offset / 512)
									{
										HWND hWnd = GetConsoleWindow();
										if (hWnd != NULL)
										{
											SetForegroundWindow(hWnd);
											BringWindowToTop(hWnd);
											SetActiveWindow(hWnd);
											SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
											cout << "是否拦截(1 拦截/0 放行):" << flush;
											cin >> IsRebused;
										}
										DeviceIoControl(hDriver, WRITE_DISKHIPS_DATA, &IsRebused, sizeof(IsRebused), NULL, 0, &ReturnedLength, NULL);
										break;
									}
								}

								cout << endl << endl;
							}
						}
					}
					else
						cout << "无法打开事件! 错误码: " << GetLastError() << endl;
				}
				else
					cout << "无法发送启动信息! 错误码: " << GetLastError() << endl;
			}
			else
				cout << "无法发送控制信息! 错误码: " << GetLastError() << endl;
		}
		else
			cout << "无法获得保护位置! 错误码: " << GetLastError() << endl;
	}
	else
		cout << "无法打开驱动! 错误码: " << GetLastError() << endl;

	_getch();

	return 0;
}