#include "OldList.hpp"

VOID
InitializeListHead(
	IN PLIST_ENTRY ListHead
)
{
	ListHead->Flink = ListHead->Blink = ListHead;
}

// This function resets the links for an entry from a doubly linked
// [Entry] Pointer to the entry
VOID
RemoveEntryList(
	IN PLIST_ENTRY Entry
)
{
	PLIST_ENTRY Blink;
	PLIST_ENTRY Flink;

	Flink = Entry->Flink;
	Blink = Entry->Blink;
	Blink->Flink = Flink;
	Flink->Blink = Blink;
}

// This function inserts an entry at the tail of a doubly-linked
// [Entry] Pointer to an entry to be inserted in the list.

VOID
InsertTailList(
	IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry
)
{
	PLIST_ENTRY Blink;

	Blink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = Blink;
	Blink->Flink = Entry;
	ListHead->Blink = Entry;
}

// This function inserts an entry at the head of a doubly-linked
// [Entry] Pointer to an entry to be inserted in the list.

VOID
InsertHeadList(
	IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry
)
{
	PLIST_ENTRY Flink;

	Flink = ListHead->Flink;
	Entry->Flink = Flink;
	Entry->Blink = ListHead;
	Flink->Blink = Entry;
	ListHead->Flink = Entry;
}

PLIST_ENTRY
RemoveHeadList(
	IN PLIST_ENTRY ListHead
)
{
	PLIST_ENTRY Flink;
	PLIST_ENTRY Entry;

	Entry = ListHead->Flink;
	Flink = Entry->Flink;
	ListHead->Flink = Flink;
	Flink->Blink = ListHead;
	return Entry;
}