#include "common.h"
#include "shell.h"

#ifndef NULL
#define NULL	( ( void* )0 )
#endif

int init_entry_list(SHELL_ENTRY_LIST* list)
{
	memset(list, 0, sizeof(SHELL_ENTRY_LIST));
	return 0;
}

int add_entry_list(SHELL_ENTRY_LIST* list, SHELL_ENTRY* entry)
{
	SHELL_ENTRY_LIST_ITEM*	newItem;

	newItem = (SHELL_ENTRY_LIST_ITEM*)malloc(sizeof(SHELL_ENTRY_LIST_ITEM));
	newItem->entry = *entry;
	newItem->next = NULL;

	if (list->count == 0)
		list->first = list->last = newItem;
	else
	{
		list->last->next = newItem;
		list->last = newItem;
	}

	list->count++;

	return 0;
}

void release_entry_list(SHELL_ENTRY_LIST* list)
{
	SHELL_ENTRY_LIST_ITEM*	currentItem;
	SHELL_ENTRY_LIST_ITEM*	nextItem;

	if (list->count == 0)
		return;

	nextItem = list->first;

	do
	{
		currentItem = nextItem;
		nextItem = currentItem->next;
		free(currentItem);
	} while (nextItem);

	list->count = 0;
	list->first = NULL;
	list->last = NULL;
}

