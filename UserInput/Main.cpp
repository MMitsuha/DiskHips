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

	WCHAR DosDevice[4] = { 0 };       //dos�豸����󳤶�Ϊ4
	WCHAR NtPath[64] = { 0 };       //nt�豸����󳤶�Ϊ64
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
		//cout << "�Ƿ�����(1 ����/0 ����):" << flush;
		//cin >> IsRebused;
		wstringstream Info;
		Info << L"�г�����ͼ�޸Ľ�ֹ����,���������鿴����!" << endl << endl;
		Info << L"------------------------------" << endl;
		if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
			Info << L"������: " << L"System" << endl;
		else
			Info << L"������: " << pUserBuffer->wstrProcessName << endl;
		Info << L"PID: " << (UINT64)pUserBuffer->hProcID << endl;
		if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
			Info << L"����·��: " << L"System" << endl;
		else
		{
			WCHAR DosPath[MAX_PATH + 4] = { 0 };
			NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
			Info << L"����·��: " << DosPath << endl;
		}
		Info << L"�������ƫ��(�ֽ�): " << pUserBuffer->Offset % 512 << endl;
		Info << L"����(512�ֽ�): " << pUserBuffer->Offset / 512 << endl;
		Info << L"����(�ֽ�): " << pUserBuffer->Length << endl;
		Info << L"Irp��ַ: " << pUserBuffer->pIrp << endl << endl;
		Info << L"(Yes: ����/No: ����)" << flush;

		if (!g_LogFile.fail())
		{
			g_Mutex.lock();
			g_LogFile << L"------------------------------" << endl;
			if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
				g_LogFile << "������: " << "System" << endl;
			else
				g_LogFile << "������: " << pUserBuffer->wstrProcessName << endl;
			g_LogFile << L"PID: " << (UINT64)pUserBuffer->hProcID << endl;
			if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
				g_LogFile << "����·��: " << "System" << endl;
			else
			{
				WCHAR DosPath[MAX_PATH + 4] = { 0 };
				NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
				g_LogFile << "����·��: " << DosPath << endl;
			}
			g_LogFile << "�������ƫ��(�ֽ�): " << pUserBuffer->Offset % 512 << endl;
			g_LogFile << "����(512�ֽ�): " << pUserBuffer->Offset / 512 << endl;
			g_LogFile << "����(�ֽ�): " << pUserBuffer->Length << endl;
			g_LogFile << "Irp��ַ: " << pUserBuffer->pIrp << endl << endl;
			g_LogFile << endl;
			g_Mutex.unlock();
		}
		else
			cerr << "[-] g_LogFileʧ��!" << endl << endl
			<< endl;

		if (!Info.fail())
		{
			if (MessageBoxW(hWnd, Info.str().c_str(), L"WARNING", MB_ICONWARNING | MB_YESNO | MB_SYSTEMMODAL) == IDNO)
				UserChoose.bIsDenied = FALSE;
		}
		else
			cerr << "[-] Infoʧ��!" << endl << endl
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
		cout << "��������ʧ��! ������: " << GetLastError() << endl;
	*/
#ifdef _WIN64
	if (!LoadNTDriver((PWSTR)L"MH", (PWSTR)L".\\DiskHips_x64.sys"))
#else
	if (!LoadNTDriver((PWSTR)L"MH", (PWSTR)L".\\DiskHips_x86.sys"))
#endif
		cout << "��������ʧ��! ������: " << GetLastError() << endl;

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
		cout << "���Զ�����: " << flush;
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
			cout << "�Ƿ����ר��ģʽ(1 ��/0 ��): " << flush;
			cin >> IsExpert;
			cin.clear();
			if (IsExpert == 49)
			{
				cin.clear();
				cout << "�������뱣��������(�ո�ָ�,Ctrl+Z����,����:0 1 2 3<Enter>): " << flush;
				istream_iterator<UINT64> cin_iter(cin), eof;
				while (cin_iter != eof)
					WarningSectors.push_back(*cin_iter++);
				cout << endl;
				cin.clear();
			}
			cin.clear();
			cout << "�Ƿ����ش���(1 ��/0 ��): " << flush;
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
								cerr << "[-] �ڴ����ʧ��!" << endl << endl
									<< endl;
								continue;
							}

							WaitForSingleObject(hEvent, INFINITE);
							bRet = DeviceIoControl(g_hDriver, READ_DISKHIPS_DATA, NULL, 0, pUserBuffer, sizeof(*pUserBuffer), &ReturnedLength, NULL);
							if (bRet)
							{
								cout << "------------------------------" << endl;
								if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
									cout << "������: " << "System" << endl;
								else
									wcout << "������: " << pUserBuffer->wstrProcessName << endl;
								cout << "PID: " << (UINT64)pUserBuffer->hProcID << endl;
								if (pUserBuffer->hProcID == (HANDLE)0 || pUserBuffer->hProcID == (HANDLE)4)
									cout << "����·��: " << "System" << endl;
								else
								{
									WCHAR DosPath[MAX_PATH + 4] = { 0 };
									NtPathToDosPathW(pUserBuffer->wstrNTProcessPath, DosPath);
									wcout << "����·��: " << DosPath << endl;
								}
								cout << "�������ƫ��(�ֽ�): " << pUserBuffer->Offset % 512 << endl;
								cout << "����(512�ֽ�): " << pUserBuffer->Offset / 512 << endl;
								cout << "����(�ֽ�): " << pUserBuffer->Length << endl;
								cout << "Irp��ַ: " << pUserBuffer->pIrp << endl << endl;
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