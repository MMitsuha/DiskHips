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

	//得到完整的驱动路径
	GetFullPathName(lpszDriverPath, MAX_PATH, szDriverImagePath, NULL);

	SC_HANDLE hServiceMgr = NULL;// SCM管理器的句柄
	SC_HANDLE hService = NULL;// NT驱动程序的服务句柄

	//打开服务控制管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		// OpenSCManager失败
		return FALSE;
	}

	// OpenSCManager成功

	//创建驱动所对应的服务
	hService = CreateService(hServiceMgr,
		lpszDriverName,             // 驱动程序的在注册表中的名字
		lpszDriverName,             // 注册表驱动程序的DisplayName 值
		SERVICE_ALL_ACCESS,         // 加载驱动程序的访问权限
		SERVICE_FILE_SYSTEM_DRIVER, // 表示加载的服务是文件系统驱动程序
		SERVICE_SYSTEM_START,       // 注册表驱动程序的Start 值
		SERVICE_ERROR_IGNORE,       // 注册表驱动程序的ErrorControl 值
		szDriverImagePath,          // 注册表驱动程序的ImagePath 值
		L"FSFilter Activity Monitor",// 注册表驱动程序的Group 值
		NULL,
		L"FltMgr",                   // 注册表驱动程序的DependOnService 值
		NULL,
		NULL);

	if (hService == NULL)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			//服务创建失败，是由于服务已经创立过
			CloseServiceHandle(hServiceMgr);    // SCM句柄
			return TRUE;
		}
		else
		{
			CloseServiceHandle(hServiceMgr);    // SCM句柄
			return FALSE;
		}
	}
	CloseServiceHandle(hService);       // 服务句柄
	CloseServiceHandle(hServiceMgr);    // SCM句柄

	//-------------------------------------------------------------------------------------------------------
	// SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances子健下的键值项
	//-------------------------------------------------------------------------------------------------------
	wcscpy(szTempStr, L"SYSTEM\\CurrentControlSet\\Services\\");
	wcscat(szTempStr, lpszDriverName);
	wcscat(szTempStr, L"\\Instances");
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTempStr, 0, (LPWSTR)L"", TRUE, KEY_ALL_ACCESS, NULL, &hKey, (LPDWORD)&dwData) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	// 注册表驱动程序的DefaultInstance 值
	wcscpy(szTempStr, lpszDriverName);
	wcscat(szTempStr, L" Instance");
	if (RegSetValueEx(hKey, L"DefaultInstance", 0, REG_SZ, (CONST BYTE*)szTempStr, (DWORD)wcslen(szTempStr)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	RegFlushKey(hKey);//刷新注册表
	RegCloseKey(hKey);
	//-------------------------------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------------------------------
	// SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances\\DriverName Instance子健下的键值项
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
	// 注册表驱动程序的Altitude 值
	wcscpy(szTempStr, lpszAltitude);
	if (RegSetValueEx(hKey, L"Altitude", 0, REG_SZ, (CONST BYTE*)szTempStr, (DWORD)wcslen(szTempStr)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	// 注册表驱动程序的Flags 值
	dwData = 0x0;
	if (RegSetValueEx(hKey, L"Flags", 0, REG_DWORD, (CONST BYTE*) & dwData, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	RegFlushKey(hKey);//刷新注册表
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
			// 服务已经开启
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
	//得到完整的驱动路径
	GetFullPathName(lpszDriverPath, MAX_PATH, szDriverImagePath, NULL);

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄

	//打开服务控制管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		bRet = FALSE;
		goto BeforeLeave;
	}

	//创建驱动所对应的服务
	hServiceDDK = CreateService(hServiceMgr,
		lpszDriverName, //驱动程序的在注册表中的名字
		lpszDriverName, // 注册表驱动程序的 DisplayName 值
		SERVICE_ALL_ACCESS, // 加载驱动程序的访问权限
		SERVICE_KERNEL_DRIVER,// 表示加载的服务是驱动程序
		SERVICE_DEMAND_START, // 注册表驱动程序的 Start 值
		SERVICE_ERROR_IGNORE, // 注册表驱动程序的 ErrorControl 值
		szDriverImagePath, // 注册表驱动程序的 ImagePath 值
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//判断服务是否失败
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}

		// 驱动程序已经加载，只需要打开
		hServiceDDK = OpenService(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}
	}

	//开启此项服务
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
				//设备被挂住
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//服务已经开启
				bRet = TRUE;
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
	//离开前关闭句柄
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

//卸载驱动程序
BOOL
UnloadNTDriver(
	IN CONST PWSTR szSvrName
)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄
	SERVICE_STATUS SvrSta;
	//打开SCM管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//带开SCM管理器失败
		bRet = FALSE;
		goto BeforeLeave;
	}
	//打开驱动所对应的服务
	hServiceDDK = OpenService(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//打开驱动所对应的服务失败
		bRet = FALSE;
		goto BeforeLeave;
	}
	//停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。
	ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);
	//动态卸载驱动程序。
	DeleteService(hServiceDDK);
	bRet = TRUE;
BeforeLeave:
	//离开前关闭打开的句柄
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