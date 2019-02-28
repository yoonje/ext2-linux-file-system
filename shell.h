#ifndef _SHELL_H_
#define _SHELL_H_

#include "disk.h"

typedef struct
{
	unsigned short	year;
	unsigned char	month;
	unsigned char	day;

	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
} SHELL_FILETIME;

typedef struct SHELL_ENTRY
{
	struct SHELL_ENTRY*	parent;

	unsigned char		name[256];
	unsigned char		isDirectory;
	unsigned int		size;

	unsigned short		permition;
	SHELL_FILETIME		createTime;
	SHELL_FILETIME		modifyTime;
	char				pdata[1024];
} SHELL_ENTRY;

typedef struct SHELL_ENTRY_LIST_ITEM
{
	struct SHELL_ENTRY				entry;
	struct SHELL_ENTRY_LIST_ITEM*	next;
} SHELL_ENTRY_LIST_ITEM;

typedef struct
{
	unsigned int					count;
	SHELL_ENTRY_LIST_ITEM*			first;
	SHELL_ENTRY_LIST_ITEM*			last;
} SHELL_ENTRY_LIST;

struct SHELL_FILE_OPERATIONS;

typedef struct SHELL_FS_OPERATIONS
{
	int	( *read_dir )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY_LIST* );
	int	( *stat )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, unsigned int*, unsigned int* );
	int ( *mkdir )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char*, SHELL_ENTRY* );
	int ( *rmdir )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char* );
	int ( *lookup )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY*, const char* );

	struct SHELL_FILE_OPERATIONS*	fileOprs;
	void*	pdata;
} SHELL_FS_OPERATIONS;

typedef struct SHELL_FILE_OPERATIONS
{
	int	( *create )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char*, SHELL_ENTRY* );
	int ( *remove )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char* );
	int	( *read )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY*, unsigned long, unsigned long, char* );
	int	( *write )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY*, unsigned long, unsigned long, const char* );
} SHELL_FILE_OPERATIONS;

typedef struct
{
	char*	name;
	int		( *mount )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, SHELL_ENTRY* );
	void	( *umount )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS* );
	int		( *format )( DISK_OPERATIONS*);
} SHELL_FILESYSTEM;

int		init_entry_list( SHELL_ENTRY_LIST* list );
int		add_entry_list( SHELL_ENTRY_LIST*, struct SHELL_ENTRY* );
void	release_entry_list( SHELL_ENTRY_LIST* );

#endif
