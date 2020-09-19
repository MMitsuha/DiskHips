#pragma once
#include "Includes.hpp"

BOOL
InstallDriver(
	IN CONST PWSTR lpszDriverName,
	IN CONST PWSTR lpszDriverPath,
	IN CONST PWSTR lpszAltitude
);

BOOL
StartDriver(
	IN CONST PWSTR lpszDriverName
);

BOOL
StopDriver(
	IN CONST PWSTR lpszDriverName
);

BOOL
DeleteDriver(
	IN CONST PWSTR lpszDriverName
);

//--------�����Ǽ���Filter������,������NT����--------

BOOL
LoadNTDriver(
	IN CONST PWSTR lpszDriverName,
	IN CONST PWSTR lpszDriverPath
);

BOOL
UnloadNTDriver(
	IN CONST PWSTR szSvrName
);