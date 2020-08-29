#pragma once
#include <windows.h>

#define CONTAINING_RECORD(address, type, field) ((type *)( \
	(PCHAR)(address) - \
	(ULONG_PTR)(&((type *)0)->field)))

#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))

VOID
InitializeListHead(
	IN PLIST_ENTRY ListHead
);

// This function resets the links for an entry from a doubly linked
// [Entry] Pointer to the entry
VOID
RemoveEntryList(
	IN PLIST_ENTRY Entry
);

// This function inserts an entry at the tail of a doubly-linked
// [Entry] Pointer to an entry to be inserted in the list.

VOID
InsertTailList(
	IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry
);

// This function inserts an entry at the head of a doubly-linked
// [Entry] Pointer to an entry to be inserted in the list.

VOID
InsertHeadList(
	IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry
);

PLIST_ENTRY
RemoveHeadList(
	IN PLIST_ENTRY ListHead
);