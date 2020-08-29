#pragma once
#include "Includes.hpp"

BOOL
InstallDriver(
	IN CONST LPWSTR lpszDriverName,
	IN CONST LPWSTR lpszDriverPath,
	IN CONST LPWSTR lpszAltitude
);

BOOL
StartDriver(
	IN CONST PWCHAR lpszDriverName
);

BOOL
StopDriver(
	IN CONST PWCHAR lpszDriverName
);

BOOL
DeleteDriver(
	IN CONST PWCHAR lpszDriverName
);

//--------�����Ǽ���Filter������,������NT����--------

BOOL
LoadNTDriver(
	IN CONST PWCHAR lpszDriverName,
	IN CONST PWCHAR lpszDriverPath
);

BOOL
UnloadNTDriver(
	IN CONST PWCHAR szSvrName
);