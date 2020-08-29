#include "Functions.hpp"
#include "OldList.hpp"

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

// 分析磁盘结构
BOOLEAN
ParseSectorOfMBR(
	IN CDiskHander& Disk,
	OUT PLIST_ENTRY Partitions,		//分区项链表
	OUT PULONG PtCount
)
{
	PPARTITION_ITEM lpExtPartition = { 0 };
	PPARTITION_ITEM lpNewPtItem = { 0 };
	BYTE lpBuf[512] = { 0 };
	ULONG Status = 0;
	PPARTITION_ENTRY lpPtEntry = { 0 };
	INT i = 0, liIndex = 0;
	ULONG luExtPtStart = 0, luCrtSec = 0;

	InitializeListHead(Partitions);
	*PtCount = 0;

	// read in main partition table,parse master boot record

	Status = Disk.Read(0, lpBuf, 1);

	while (Status == 0)
	{
		if (lpBuf[510] != 0x55 || lpBuf[511] != 0xAA ||
			lpBuf[446] != 0 && lpBuf[446] != 0x80 ||
			lpBuf[462] != 0 && lpBuf[462] != 0x80 ||
			lpBuf[478] != 0 && lpBuf[478] != 0x80 ||
			lpBuf[494] != 0 && lpBuf[494] != 0x80)
			break;
		if (lpNewPtItem == NULL)
		{
			lpNewPtItem = (PPARTITION_ITEM)LocalAlloc(NULL, sizeof(PARTITION_ITEM));
			if (lpNewPtItem == NULL)
				return FALSE;
			RtlZeroMemory(lpNewPtItem, sizeof(PARTITION_ITEM));
		}
		// record the sectors where there is the partition-table
		lpNewPtItem->PartitionType = PARTITION_ENTRY_UNUSED;
		lpNewPtItem->ExtType = PARTITION_EXT_PTTAB;
		lpNewPtItem->StartSector = luCrtSec;
		lpNewPtItem->Length = 0x10;
		lpNewPtItem->Index = liIndex++;
		InsertTailList(Partitions, &lpNewPtItem->ForPtChain.ListEntry);

		*PtCount += 1;
		lpNewPtItem = NULL;

		lpPtEntry = (PPARTITION_ENTRY)&lpBuf[446];

		for (i = 0; i < 4; i++, lpPtEntry++)
		{	// search in the major partition table
			if (lpNewPtItem == NULL)
			{
				lpNewPtItem = (PPARTITION_ITEM)LocalAlloc(NULL, sizeof(PARTITION_ITEM));
				if (lpNewPtItem == NULL)
					return FALSE;
				RtlZeroMemory(lpNewPtItem, sizeof(PARTITION_ITEM));
			}

			if (lpPtEntry->sys_ind == PARTITION_EXTENDED
				|| lpPtEntry->sys_ind == PARTITION_XINT13_EXTENDED)
			{
				lpNewPtItem->PartitionType = PARTITION_EXTENDED;
				lpNewPtItem->StartSector = lpPtEntry->start_sect + luExtPtStart;
				lpNewPtItem->Length = lpPtEntry->nr_sects;

				lpExtPartition = lpNewPtItem;
				lpNewPtItem = 0;
			}
			else
				if (lpPtEntry->sys_ind != 0)
				{
					if (lpPtEntry->boot_ind != 0 && lpPtEntry->boot_ind != 0x80)
					{
						//ms规定，active partition value must be 0 or 0x80
						continue;
					}
					lpNewPtItem->PartitionType = lpPtEntry->sys_ind;
					lpNewPtItem->StartSector = lpPtEntry->start_sect + luCrtSec;//luExtPtStart;
					lpNewPtItem->Length = lpPtEntry->nr_sects;

					lpNewPtItem->Index = liIndex++;

					InsertTailList(Partitions, &lpNewPtItem->ForPtChain.ListEntry);

					*PtCount += 1;
					lpNewPtItem = NULL;
				}
		}
		if (Status != 0)
			return FALSE;
		// search in extension partition chain
		if (lpExtPartition != NULL)
		{
			if (luExtPtStart == 0)	// just do it on first extension node
				luExtPtStart = lpExtPartition->StartSector;

			luCrtSec = lpExtPartition->StartSector;
			Status = Disk.Read(luCrtSec, lpBuf, 1);
			LocalFree(lpExtPartition);
			lpExtPartition = NULL;
		}
		else
			break;
	}

	return TRUE;
}

BOOL
cls(
	VOID
)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD CoordScreen = { 0, 0 };    // home for the cursor
	DWORD dwCharsWritten = 0;
	CONSOLE_SCREEN_BUFFER_INFO CSBI = { 0 };
	DWORD dwConSize = 0;

	// Get the number of character cells in the current buffer.

	if (!GetConsoleScreenBufferInfo(hConsole, &CSBI))
		return FALSE;

	dwConSize = CSBI.dwSize.X * CSBI.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(hConsole,        // Handle to console screen buffer
		(TCHAR)' ',     // Character to write to the buffer
		dwConSize,       // Number of cells to write
		CoordScreen,     // Coordinates of first cell
		&dwCharsWritten))// Receive number of characters written
		return FALSE;

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(hConsole, &CSBI))
		return FALSE;

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(hConsole,         // Handle to console screen buffer
		CSBI.wAttributes, // Character attributes to use
		dwConSize,        // Number of cells to set attribute
		CoordScreen,      // Coordinates of first cell
		&dwCharsWritten)) // Receive number of characters written
		return FALSE;

	// Put the cursor at its home coordinates.

	return SetConsoleCursorPosition(hConsole, CoordScreen);
}

BOOLEAN
EnableDebugPrivilege(
	VOID
)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;
	LUID sedebugnameValue = { 0 };
	TOKEN_PRIVILEGES tkp = { 0 };
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return FALSE;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{
		CloseHandle(hToken);
		return FALSE;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return FALSE;
	}
	return TRUE;
}

LSTATUS
GetDiskNumber(
	OUT PDWORD NumberOfDisk
)
{
	if (!NumberOfDisk)
		return ERROR_INVALID_PARAMETER;

	LSTATUS Status = ERROR_SUCCESS;
	HKEY hKeyEnum = NULL;
	Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\disk\\Enum", 0, KEY_QUERY_VALUE, &hKeyEnum);
	if (Status == ERROR_SUCCESS)
	{
		DWORD DataLength = sizeof(*NumberOfDisk);
		DWORD Type = REG_DWORD;
		Status = RegQueryValueExW(hKeyEnum, L"Count", NULL, &Type, (PBYTE)NumberOfDisk, &DataLength);
		if (Status != ERROR_SUCCESS)
		{
			RegCloseKey(hKeyEnum);
			return Status;
		}
		Status = RegCloseKey(hKeyEnum);
	}

	return Status;
}

BOOLEAN
RecordSectors(
	IN CDiskHander& DiskObject,
	OUT list<UINT64>& List
)
{
	LIST_ENTRY Partitions = { 0 };
	ULONG Count = 0;

	if (ParseSectorOfMBR(DiskObject, &Partitions, &Count))
	{
		// parse partition table
		PLIST_ENTRY NextEntry = { 0 };
		PPARTITION_ITEM PtItem = NULL;
		/*
		USHORT lsType[8] = { 0 };
		*/

		INT SequeceVol = 1;

		while (!IsListEmpty(&Partitions))
		{
			NextEntry = RemoveHeadList(&Partitions);
			if (NextEntry == NULL || NextEntry == &Partitions)
				break;
			PtItem = (PPARTITION_ITEM)CONTAINING_RECORD(NextEntry, PARTITION_ITEM, ForPtChain.ListEntry);

			/*
			switch (PtItem->PartitionType)
			{
				case PARTITION_FAT_12:
					memcpy(lsType, L"FAT12", sizeof(L"FAT12"));
					break;

				case PARTITION_FAT_16:
					memcpy(lsType, L"FAT16", sizeof(L"FAT16"));
					break;

				case PARTITION_HUGE:
				case PARTITION_FAT32:
				case PARTITION_FAT32_XINT13:
				case PARTITION_XINT13:
					memcpy(lsType, L"FAT32", sizeof(L"FAT32"));
					break;

				case PARTITION_IFS:
					memcpy(lsType, L"NTFS", sizeof(L"NTFS"));
					break;

				default:
					RtlZeroMemory(lsType, sizeof(lsType));
					break;
			}
			*/

			/*
			if (lsType[0] != 0)
			{
			*/
			List.push_back(PtItem->StartSector);

			SequeceVol++;
			/*
			}
			*/

			/*
			RtlZeroMemory(lsType, sizeof(lsType));
			*/
		}
	}
	else
	{
		cerr << "[-] Parse Disk Error" << endl;
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
RecordAllSectors(
	OUT list<list<UINT64>>& Object
)
{
	DWORD NumberOfDisk = 0;
	if (GetDiskNumber(&NumberOfDisk) != ERROR_SUCCESS)
		return FALSE;
	if (!NumberOfDisk)
		return FALSE;

	for (DWORD i = 0; i < NumberOfDisk; i++)
	{
		WCHAR DiskLinkName[MAX_PATH] = { 0 };
		swprintf_s(DiskLinkName, MAX_PATH, L"\\\\.\\PhysicalDrive%u", i);
		//wcout << L"DiskLinkName: " << DiskLinkName << endl;

		CDiskHander DiskObject;
		if (!DiskObject.Attach(DiskLinkName))
		{
			list<UINT64> List;
			if (!RecordSectors(DiskObject, List))
			{
				cerr << "[-] Record Disk Error" << endl;
				return FALSE;
			}
			Object.push_back(List);
			DiskObject.Detach();
		}
		else
			cerr << "[-] Open Disk Error" << endl;
	}

	return TRUE;
}