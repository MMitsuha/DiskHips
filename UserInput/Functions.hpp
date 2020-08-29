#pragma once
#include "Includes.hpp"
#include "Structs.hpp"
#include "DiskHander.hpp"
#include "Functions.hpp"
#include <list>
#include <algorithm>

BOOLEAN
ParseSectorOfMBR(
	IN CDiskHander& Disk,
	OUT PLIST_ENTRY Partitions,	//分区项链表
	OUT PULONG PtCount
);

BOOL
cls(
	VOID
);

BOOLEAN
EnableDebugPrivilege(
	VOID
);

LSTATUS
GetDiskNumber(
	OUT PDWORD NumberOfDisk
);

BOOLEAN
RecordSectors(
	IN CDiskHander& DiskObject,
	OUT list<UINT64>& List
);

BOOLEAN
RecordAllSectors(
	OUT list<list<UINT64>>& Object
);