#pragma once
#include <windows.h>

#pragma pack(1)
typedef struct _PARTITION_ENTRY
{
	UCHAR boot_ind;		/* 0x80 - active */
	UCHAR head;			/* starting head */
	UCHAR sector;		/* starting sector */
	UCHAR cyl;			/* starting cylinder */
	UCHAR sys_ind;		/* What partition type */
	UCHAR end_head;		/* end head */
	UCHAR end_sector;	/* end sector */
	UCHAR end_cyl;		/* end cylinder */
	ULONG start_sect;	/* starting sector counting from 0 */
	ULONG nr_sects;		/* nr of sectors in partition, length */
}PARTITION_ENTRY, * PPARTITION_ENTRY;
#pragma pack()

typedef struct _PARTITION_ITEM
{
	ULONG  StartSector;
	ULONG  Length;
	UCHAR  PartitionType;
	UCHAR  ProtectType;
	UCHAR  ExtType;
	UCHAR  Index;
	union {
		struct {
			PVOID Context;
		}ForIoctl;
		struct {
			LIST_ENTRY ListEntry;
		}ForPtChain;
	};
}PARTITION_ITEM, * PPARTITION_ITEM;

// PartitionType
#ifndef PARTITION_ENTRY_UNUSED
#define PARTITION_ENTRY_UNUSED          0x00      // Entry unused
#define PARTITION_FAT_12                0x01      // 12-bit FAT entries
#define PARTITION_XENIX_1               0x02      // Xenix
#define PARTITION_XENIX_2               0x03      // Xenix
#define PARTITION_FAT_16                0x04      // 16-bit FAT entries
#define PARTITION_EXTENDED              0x05      // Extended partition entry
#define PARTITION_HUGE                  0x06      // Huge partition MS-DOS V4
#define PARTITION_IFS                   0x07      // IFS Partition
#define PARTITION_OS2BOOTMGR            0x0A      // OS/2 Boot Manager/OPUS/Coherent swap
#define PARTITION_FAT32                 0x0B      // FAT32
#define PARTITION_FAT32_XINT13          0x0C      // FAT32 using extended int13 services
#define PARTITION_XINT13                0x0E      // Win95 partition using extended int13 services
#define PARTITION_XINT13_EXTENDED       0x0F      // Same as type 5 but uses extended int13 services
#define PARTITION_PREP                  0x41      // PowerPC Reference Platform (PReP) Boot Partition
#define PARTITION_LDM                   0x42      // Logical Disk Manager partition
#define PARTITION_UNIX                  0x63      // Unix
#endif//default partition types

// ExtType
#define PARTITION_EXT_PTTAB			5	//
#define PARTITION_EXT_FREESPC		6   //
#define PARTITION_EXT_ACTIVE		80  // 活动分区

// ProtectType
#define PARTITION_PROTECT_READONLY  1
#define PARTITION_PROTECT_NORMAL	2
#define PARTITION_PROTECT_NO		255
#pragma pack()