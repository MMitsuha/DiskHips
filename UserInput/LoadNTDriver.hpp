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

//--------上面是加载Filter驱动的,下面是NT驱动--------

BOOL
LoadNTDriver(
	IN CONST PWCHAR lpszDriverName,
	IN CONST PWCHAR lpszDriverPath
);

BOOL
UnloadNTDriver(
	IN CONST PWCHAR szSvrName
);