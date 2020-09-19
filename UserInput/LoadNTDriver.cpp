#include "LoadNTDriver.hpp"

BOOL
InstallDriver(
	IN CONST PWSTR lpszDriverName,
	IN CONST PWSTR lpszDriverPath,
	IN CONST PWSTR lpszAltitude
)
{
	WCHAR szTempStr[MAX_PATH];
	HKEY hKey;
	DWORD dwData;
	WCHAR szDriverImagePath[MAX_PATH];

	if (NULL == lpszDriverName || NULL == lpszDriverPath)
		return FALSE;

	//�õ�����������·��
	GetFullPathName(lpszDriverPath, MAX_PATH, szDriverImagePath, NULL);

	SC_HANDLE hServiceMgr = NULL;// SCM�������ľ��
	SC_HANDLE hService = NULL;// NT��������ķ�����

	//�򿪷�����ƹ�����
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		// OpenSCManagerʧ��
		return FALSE;
	}

	// OpenSCManager�ɹ�

	//������������Ӧ�ķ���
	hService = CreateService(hServiceMgr,
		lpszDriverName,             // �����������ע����е�����
		lpszDriverName,             // ע������������DisplayName ֵ
		SERVICE_ALL_ACCESS,         // ������������ķ���Ȩ��
		SERVICE_FILE_SYSTEM_DRIVER, // ��ʾ���صķ������ļ�ϵͳ��������
		SERVICE_SYSTEM_START,       // ע������������Start ֵ
		SERVICE_ERROR_IGNORE,       // ע������������ErrorControl ֵ
		szDriverImagePath,          // ע������������ImagePath ֵ
		L"FSFilter Activity Monitor",// ע������������Group ֵ
		NULL,
		L"FltMgr",                   // ע������������DependOnService ֵ
		NULL,
		NULL);

	if (hService == NULL)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			//���񴴽�ʧ�ܣ������ڷ����Ѿ�������
			CloseServiceHandle(hServiceMgr);    // SCM���
			return TRUE;
		}
		else
		{
			CloseServiceHandle(hServiceMgr);    // SCM���
			return FALSE;
		}
	}
	CloseServiceHandle(hService);       // ������
	CloseServiceHandle(hServiceMgr);    // SCM���

	//-------------------------------------------------------------------------------------------------------
	// SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances�ӽ��µļ�ֵ��
	//-------------------------------------------------------------------------------------------------------
	wcscpy(szTempStr, L"SYSTEM\\CurrentControlSet\\Services\\");
	wcscat(szTempStr, lpszDriverName);
	wcscat(szTempStr, L"\\Instances");
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTempStr, 0, (LPWSTR)L"", TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	// ע������������DefaultInstance ֵ
	wcscpy(szTempStr, lpszDriverName);
	wcscat(szTempStr, L" Instance");
	if (RegSetValueEx(hKey, L"DefaultInstance", 0, REG_SZ, (CONST BYTE*)szTempStr, (DWORD)wcslen(szTempStr)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	RegFlushKey(hKey);//ˢ��ע���
	RegCloseKey(hKey);
	//-------------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------------
	// SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances\\DriverName Instance�ӽ��µļ�ֵ��
	//-------------------------------------------------------------------------------------------------------
	wcscpy(szTempStr, L"SYSTEM\\CurrentControlSet\\Services\\");
	wcscat(szTempStr, lpszDriverName);
	wcscat(szTempStr, L"\\Instances\\");
	wcscat(szTempStr, lpszDriverName);
	wcscat(szTempStr, L" Instance");
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTempStr, 0, (LPWSTR)L"", TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	// ע������������Altitude ֵ
	wcscpy(szTempStr, lpszAltitude);
	if (RegSetValueEx(hKey, L"Altitude", 0, REG_SZ, (CONST BYTE*)szTempStr, (DWORD)wcslen(szTempStr)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	// ע������������Flags ֵ
	dwData = 0x0;
	if (RegSetValueEx(hKey, L"Flags", 0, REG_DWORD, (CONST BYTE*) & dwData, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	RegFlushKey(hKey);//ˢ��ע���
	RegCloseKey(hKey);
	//-------------------------------------------------------------------------------------------------------

	return TRUE;
}

BOOL
StartDriver(
	IN CONST PWSTR lpszDriverName
)
{
	SC_HANDLE schManager;
	SC_HANDLE schService;

	if (NULL == lpszDriverName)
	{
		return FALSE;
	}

	schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schManager)
	{
		return FALSE;
	}
	schService = OpenService(schManager, lpszDriverName, SERVICE_ALL_ACCESS);
	if (NULL == schService)
	{
		CloseServiceHandle(schManager);
		return FALSE;
	}

	if (!StartService(schService, 0, NULL))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schManager);
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
			// �����Ѿ�����
			return TRUE;
		}
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schManager);

	return TRUE;
}

BOOL
StopDriver(
	IN CONST PWSTR lpszDriverName
)
{
	SC_HANDLE        schManager;
	SC_HANDLE        schService;
	SERVICE_STATUS    svcStatus;
	bool            bStopped = false;

	schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schManager)
	{
		return FALSE;
	}
	schService = OpenService(schManager, lpszDriverName, SERVICE_ALL_ACCESS);
	if (NULL == schService)
	{
		CloseServiceHandle(schManager);
		return FALSE;
	}
	if (!ControlService(schService, SERVICE_CONTROL_STOP, &svcStatus) && (svcStatus.dwCurrentState != SERVICE_STOPPED))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schManager);
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schManager);

	return TRUE;
}

BOOL
DeleteDriver(
	IN CONST PWSTR lpszDriverName
)
{
	SC_HANDLE        schManager;
	SC_HANDLE        schService;
	SERVICE_STATUS    svcStatus;

	schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schManager)
	{
		return FALSE;
	}
	schService = OpenService(schManager, lpszDriverName, SERVICE_ALL_ACCESS);
	if (NULL == schService)
	{
		CloseServiceHandle(schManager);
		return FALSE;
	}
	ControlService(schService, SERVICE_CONTROL_STOP, &svcStatus);
	if (!DeleteService(schService))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schManager);
		return FALSE;
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(schManager);

	return TRUE;
}

BOOL
LoadNTDriver(
	IN CONST PWSTR lpszDriverName,
	IN CONST PWSTR lpszDriverPath
)
{
	WCHAR szDriverImagePath[MAX_PATH];
	//�õ�����������·��
	GetFullPathName(lpszDriverPath, MAX_PATH, szDriverImagePath, NULL);

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM�������ľ��
	SC_HANDLE hServiceDDK = NULL;//NT��������ķ�����

	//�򿪷�����ƹ�����
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		bRet = FALSE;
		goto BeforeLeave;
	}

	//������������Ӧ�ķ���
	hServiceDDK = CreateService(hServiceMgr,
		lpszDriverName, //�����������ע����е�����
		lpszDriverName, // ע������������ DisplayName ֵ
		SERVICE_ALL_ACCESS, // ������������ķ���Ȩ��
		SERVICE_KERNEL_DRIVER,// ��ʾ���صķ�������������
		SERVICE_DEMAND_START, // ע������������ Start ֵ
		SERVICE_ERROR_IGNORE, // ע������������ ErrorControl ֵ
		szDriverImagePath, // ע������������ ImagePath ֵ
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//�жϷ����Ƿ�ʧ��
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}

		// ���������Ѿ����أ�ֻ��Ҫ��
		hServiceDDK = OpenService(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}
	}

	//�����������
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)
			{
				//�豸����ס
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//�����Ѿ�����
				bRet = TRUE;
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
	//�뿪ǰ�رվ��
BeforeLeave:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

//ж����������
BOOL
UnloadNTDriver(
	IN CONST PWSTR szSvrName
)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM�������ľ��
	SC_HANDLE hServiceDDK = NULL;//NT��������ķ�����
	SERVICE_STATUS SvrSta;
	//��SCM������
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//����SCM������ʧ��
		bRet = FALSE;
		goto BeforeLeave;
	}
	//����������Ӧ�ķ���
	hServiceDDK = OpenService(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//����������Ӧ�ķ���ʧ��
		bRet = FALSE;
		goto BeforeLeave;
	}
	//ֹͣ�����������ֹͣʧ�ܣ�ֻ�������������ܣ��ٶ�̬���ء�
	ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);
	//��̬ж����������
	DeleteService(hServiceDDK);
	bRet = TRUE;
BeforeLeave:
	//�뿪ǰ�رմ򿪵ľ��
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}