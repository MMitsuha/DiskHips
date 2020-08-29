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

	WCHAR DosDevice[4] = { 0 };       //dos�豸����󳤶�Ϊ4
	WCHAR NtPath[64] = { 0 };       //nt�豸����󳤶�Ϊ64
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
		cout << "��������ʧ��! ������:" << GetLastError() << endl;

	HANDLE hDriver = CreateFileW(USER_MBRHIPS_LINK_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDriver != INVALID_HANDLE_VALUE)
	{
		BOOL bRet = TRUE;
		DWORD ReturnedLength = 0;
		list<list<UINT64>> Object;
		list<UINT64> WarningSectors;
		cout << "���Զ�����:" << flush;
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
			cout << "�Ƿ����ר��ģʽ(1 ��/0 ��):" << flush;
			cin >> IsExpert;
			if (IsExpert == 49)
			{
				cin.clear();
				cout << "�������뱣��������(�ո�ָ�,Ctrl+Z����,����:0 1 2 3^Z<Enter>):" << flush;
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
									wcout << "������: " << L"System" << endl;
								else
									wcout << "������: " << UserBuffer.wstrProcessName << endl;
								cout << "PID: " << (UINT64)UserBuffer.hProcID << endl;
								if (UserBuffer.hProcID == (HANDLE)0 || UserBuffer.hProcID == (HANDLE)4)
									wcout << "����·��: " << L"System" << endl;
								else
								{
									WCHAR DosPath[MAX_PATH + 4] = { 0 };
									NtPathToDosPathW(UserBuffer.wstrNTProcessPath, DosPath);
									wcout << "����·��: " << DosPath << endl;
								}
								cout << "�������ƫ��(�ֽ�): " << UserBuffer.Offset % 512 << endl;
								cout << "����(512�ֽ�): " << UserBuffer.Offset / 512 << endl;
								cout << "����(�ֽ�): " << UserBuffer.Length << endl;

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
											cout << "�Ƿ�����(1 ����/0 ����):" << flush;
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
						cout << "�޷����¼�! ������: " << GetLastError() << endl;
				}
				else
					cout << "�޷�����������Ϣ! ������: " << GetLastError() << endl;
			}
			else
				cout << "�޷����Ϳ�����Ϣ! ������: " << GetLastError() << endl;
		}
		else
			cout << "�޷���ñ���λ��! ������: " << GetLastError() << endl;
	}
	else
		cout << "�޷�������! ������: " << GetLastError() << endl;

	_getch();

	return 0;
}