#include "Main.hpp"
#include "Functions.hpp"
#include <list>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <mutex>

HANDLE g_hDriver = INVALID_HANDLE_VALUE;
BOOLEAN g_IsHide = FALSE;
wofstream g_LogFile("Warns.log", ios::app);
mutex g_Mutex;

BOOLEAN
NtPathToDosPathW(
	IN PWSTR FullNtPath,
	OUT PWSTR FullDosPath
)
{
	if (!FullNtPath || !FullDosPath)
		return FALSE;

	WCHAR DosDevice[4] = { 0 };       //dos设备名最大长度为4
	WCHAR NtPath[64] = { 0 };       //nt设备名最大长度为64
	SIZE_T NtPathLen = 0;

	for (SHORT i = 65; i < 26 + 65; i++)
	{
		DosDevice[0] = i;
		DosDevice[1] = L':';
		if (QueryDosDeviceW(DosDevice, NtPath, 64))
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
	return FALSE;
}

VOID
SafeGetNativeSystemInfo(
	OUT LPSYSTEM_INFO lpSystemInfo
)
{
	if (!lpSystemInfo)
		return;

	Func_GetSystemInfo funcGetNativeSystemInfo = (Func_GetSystemInfo)GetProcAddress(GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");

	if (funcGetNativeSystemInfo)
		funcGetNativeSystemInfo(lpSystemInfo);
	else
		GetSystemInfo(lpSystemInfo);
}

DWORD
GetSystemBits(
	VOID
)
{
	SYSTEM_INFO SystemInfo;
	SafeGetNativeSystemInfo(&SystemInfo);
	if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		return 0x64;
	else
		return 0x86;
}

DWORD
WINAPI
WarningThread(
	IN PVOID pBuffer
)
{
	DWORD ReturnedLength = 0;
	PUSER_BUFFER pUserBuffer = (PUSER_BUFFER)pBuffer;
	USER_CHOOSE UserChoose = { pUserBuffer->pIrp, TRUE };
	HWND hWnd = GetConsoleWindow();
	if (hWnd != NULL)
	{
		/*
		SetForegroundWindow(hWnd);
		BringWindowToTop(hWnd);
		SetActiveWindow(hWnd);
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		*/
		//cout << "是否拦截(1 拦截/0 放行):" << flush;
		//cin >> IsRebused;
		wstringstream Info;
		Info << L"有程序试图修改禁止扇区,请打开主程序查看详情!" << endl << endl;
		Info << L"------------------------------" << endl;
		if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
			Info << L"进程名: " << L"System" << endl;
		else
			Info << L"进程名: " << pUserBuffer->wstrProcessName << endl;
		Info << L"PID: " << (UINT64)pUserBuffer->hProcID << endl;
		if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
			Info << L"进程路径: " << L"System" << endl;
		else
		{
			WCHAR DosPath[MAX_PATH + 4] = { 0 };
			NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
			Info << L"进程路径: " << DosPath << endl;
		}
		Info << L"相对扇区偏移(字节): " << pUserBuffer->Offset % 512 << endl;
		Info << L"扇区(512字节): " << pUserBuffer->Offset / 512 << endl;
		Info << L"长度(字节): " << pUserBuffer->Length << endl;
		Info << L"Irp地址: " << pUserBuffer->pIrp << endl << endl;
		Info << L"(Yes: 拦截/No: 放行)" << flush;

		if (!g_LogFile.fail())
		{
			g_Mutex.lock();
			g_LogFile << L"------------------------------" << endl;
			if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
				g_LogFile << "进程名: " << "System" << endl;
			else
				g_LogFile << "进程名: " << pUserBuffer->wstrProcessName << endl;
			g_LogFile << L"PID: " << (UINT64)pUserBuffer->hProcID << endl;
			if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
				g_LogFile << "进程路径: " << "System" << endl;
			else
			{
				WCHAR DosPath[MAX_PATH + 4] = { 0 };
				NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
				g_LogFile << "进程路径: " << DosPath << endl;
			}
			g_LogFile << "相对扇区偏移(字节): " << pUserBuffer->Offset % 512 << endl;
			g_LogFile << "扇区(512字节): " << pUserBuffer->Offset / 512 << endl;
			g_LogFile << "长度(字节): " << pUserBuffer->Length << endl;
			g_LogFile << "Irp地址: " << pUserBuffer->pIrp << endl << endl;
			g_LogFile << endl;
			g_Mutex.unlock();
		}
		else
			cerr << "[-] g_LogFile失败!" << endl << endl
			<< endl;

		if (!Info.fail())
		{
			if (MessageBoxW(hWnd, Info.str().c_str(), L"WARNING", MB_ICONWARNING | MB_YESNO | MB_SYSTEMMODAL) == IDNO)
				UserChoose.bIsDenied = FALSE;
		}
		else
			cerr << "[-] Info失败!" << endl << endl
			<< endl;
	}
	delete pUserBuffer;
	if (g_IsHide == 49)
		ShowWindow(hWnd, SW_HIDE);
	return DeviceIoControl(g_hDriver, WRITE_DISKHIPS_DATA, &UserChoose, sizeof(UserChoose), NULL, 0, &ReturnedLength, NULL);
}

INT
main(
	_In_ UINT argc,
	_In_ CHAR** argv
)
{
	/*
	WCHAR wstrDriverName[18] = { 0 };
	if (GetSystemBits() == 0x64)
		RtlCopyMemory(wstrDriverName, L".\\DiskHips_x64.sys", sizeof(L".\\DiskHips_x64.sys"));
	else
		RtlCopyMemory(wstrDriverName, L".\\DiskHips_x86.sys", sizeof(L".\\DiskHips_x86.sys"));
	if (!LoadNTDriver((PWCHAR)L"MH", wstrDriverName))
		cout << "启动驱动失败! 错误码: " << GetLastError() << endl;
	*/
#ifdef _WIN64
	if (!LoadNTDriver((PWSTR)L"MH", (PWSTR)L".\\DiskHips_x64.sys"))
#else
	if (!LoadNTDriver((PWSTR)L"MH", (PWSTR)L".\\DiskHips_x86.sys"))
#endif
		cout << "启动驱动失败! 错误码: " << GetLastError() << endl;

	g_hDriver = CreateFileW(USER_MBRHIPS_LINK_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (g_hDriver != INVALID_HANDLE_VALUE)
	{
		BOOL bRet = TRUE;
		DWORD ReturnedLength = 0;
		/*
		ULONG_PTR PID = 0;
		cin >> PID;
		bRet = DeviceIoControl(hDriver, TEST, &PID, sizeof(PID), NULL, 0, &ReturnedLength, NULL);
		_getch();
		*/
		list<list<UINT64>> Object;
		list<UINT64> WarningSectors;
		cout << "已自动保护: " << flush;
		if (RecordAllSectors(Object))
		{
			for (auto CurrentList : Object)
				for (auto CurrentSector : CurrentList)
				{
					cout << CurrentSector << "," << flush;
					WarningSectors.push_back(CurrentSector);
				}
			cout << endl;

			cin.clear();
			BOOLEAN IsExpert = FALSE;
			cout << "是否进入专家模式(1 是/0 否): " << flush;
			cin >> IsExpert;
			cin.clear();
			if (IsExpert == 49)
			{
				cin.clear();
				cout << "输入你想保护的扇区(空格分隔,Ctrl+Z结束,例子:0 1 2 3<Enter>): " << flush;
				istream_iterator<UINT64> cin_iter(cin), eof;
				while (cin_iter != eof)
					WarningSectors.push_back(*cin_iter++);
				cout << endl;
				cin.clear();
			}
			cin.clear();
			cout << "是否隐藏窗口(1 是/0 否): " << flush;
			cin >> g_IsHide;
			cin.clear();
			if (g_IsHide == 49)
			{
				HWND hWnd = GetConsoleWindow();
				ShowWindow(hWnd, SW_HIDE);
			}
			for (auto CurrentSector : WarningSectors)
			{
				BOOL bStatus = DeviceIoControl(g_hDriver, ADD_DENY_SECTOR, &CurrentSector, sizeof(CurrentSector), NULL, 0, &ReturnedLength, NULL);
				if (!bStatus)
				{
					bRet = FALSE;
					break;
				}
			}

			if (bRet)
			{
				bRet = DeviceIoControl(g_hDriver, ENABLE_DISKHIPS_MONITOR, NULL, 0, NULL, 0, &ReturnedLength, NULL);
				if (bRet)
				{
					HANDLE hEvent = OpenEventW(SYNCHRONIZE, FALSE, USER_MBRHIPS_EVENT_NAME);
					if (hEvent != INVALID_HANDLE_VALUE)
					{
						while (TRUE)
						{
							PUSER_BUFFER pUserBuffer = new USER_BUFFER{ 0 };
							if (pUserBuffer == NULL)
							{
								cerr << "[-] 内存分配失败!" << endl << endl
									<< endl;
								continue;
							}

							WaitForSingleObject(hEvent, INFINITE);
							bRet = DeviceIoControl(g_hDriver, READ_DISKHIPS_DATA, NULL, 0, pUserBuffer, sizeof(*pUserBuffer), &ReturnedLength, NULL);
							if (bRet)
							{
								cout << "------------------------------" << endl;
								if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
									cout << "进程名: " << "System" << endl;
								else
									wcout << "进程名: " << pUserBuffer->wstrProcessName << endl;
								cout << "PID: " << (UINT64)pUserBuffer->hProcID << endl;
								if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
									cout << "进程路径: " << "System" << endl;
								else
								{
									WCHAR DosPath[MAX_PATH + 4] = { 0 };
									NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
									wcout << "进程路径: " << DosPath << endl;
								}
								cout << "相对扇区偏移(字节): " << pUserBuffer->Offset % 512 << endl;
								cout << "扇区(512字节): " << pUserBuffer->Offset / 512 << endl;
								cout << "长度(字节): " << pUserBuffer->Length << endl;
								cout << "Irp地址: " << pUserBuffer->pIrp << endl << endl;
								cout << endl;

								for (auto CurrentSector : WarningSectors)
								{
									if (CurrentSector == pUserBuffer->Offset / 512)
									{
										CreateThread(NULL, 0, WarningThread, pUserBuffer, 0, 0);
										break;
									}
								}
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