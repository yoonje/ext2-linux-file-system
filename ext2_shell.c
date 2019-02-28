#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ext2_shell.h"
typedef struct {
	char * address;
}DISK_MEMORY;

static SHELL_FILE_OPERATIONS g_file =
{
	fs_create,
	fs_remove,
	fs_read,
	fs_write
};

static SHELL_FS_OPERATIONS g_fsOprs =
{
	fs_read_dir,
	NULL,
	fs_mkdir,
	fs_rmdir,
	fs_lookup,
	&g_file,
	NULL
};

static SHELL_FILESYSTEM g_fat =
{
	"EXT2",
	fs_mount,
	fs_umount,
	fs_format
};
/******************************************************************************/
/*																			  */
/*								FS_COMMAND	 								  */
/*																			  */
/******************************************************************************/
int fs_mount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* root)
{
	EXT2_FILESYSTEM* fs;
	EXT2_NODE ext2_entry;
	int result;

	*fsOprs = g_fsOprs;

	fsOprs->pdata = malloc(sizeof(EXT2_FILESYSTEM));
	fs = FSOPRS_TO_EXT2FS(fsOprs);
	ZeroMemory(fs, sizeof(EXT2_FILESYSTEM));
	fs->disk = disk;
	
	result = read_superblock(fs, &ext2_entry);
	
	if (result == EXT2_SUCCESS)
	{
		printf("number of groups         : %d\n", NUMBER_OF_GROUPS);
		printf("blocks per group         : %d\n", fs->sb.block_per_group);
		printf("bytes per block          : %d\n", disk->bytesPerSector);
		printf("free block count	 : %d\n", fs->sb.free_block_count);
		printf("free inode count	 : %d\n", fs->sb.free_inode_count);
		printf("first non reserved inode : %d\n", fs->sb.first_non_reserved_inode);
		printf("inode structure size	 : %d\n", fs->sb.inode_structure_size);
		printf("first data block number  : %d\n\n", fs->sb.first_data_block_each_group);
	}
	
	ext2_entry_to_shell_entry(&ext2_entry, root);

	return result;
}

void fs_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs)
{
	return;
}

int fs_format(DISK_OPERATIONS* disk)
{
	printf("formatting as a EXT2\n");
	ext2_format(disk);

	return  1;
}

int fs_read( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer )
{
    
    EXT2_NODE    EXT2Entry;
    
    shell_entry_to_ext2_entry( entry, &EXT2Entry );
    
    return ext2_read( &EXT2Entry, offset, length, buffer );
}

int fs_write(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer)
{
	EXT2_NODE EXT2Entry;

	shell_entry_to_ext2_entry(entry, &EXT2Entry);

	return ext2_write(&EXT2Entry, offset, length, buffer);
}

int	fs_create(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;
	
	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	result = ext2_create(&EXT2Parent, name, &EXT2Entry);

	ext2_entry_to_shell_entry(&EXT2Entry, retEntry);

	return result;
}

int fs_mkdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE      EXT2_Parent;
	EXT2_NODE      EXT2_Entry;
	int               result;

	if (is_exist(disk, fsOprs, parent, name))
		return EXT2_ERROR;

	shell_entry_to_ext2_entry(parent, &EXT2_Parent);

	result = ext2_mkdir(&EXT2_Parent, name, &EXT2_Entry);

	ext2_entry_to_shell_entry(&EXT2_Entry, retEntry);

	return result;
}

int fs_lookup(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);
	
	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry))
		return result;
	
	ext2_entry_to_shell_entry(&EXT2Entry, entry);

	return result;
}

int fs_read_dir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY_LIST* list)
{
	EXT2_NODE   entry;

	if (list->count)
		release_entry_list(list);

	shell_entry_to_ext2_entry(parent, &entry);
	
	ext2_read_dir(&entry, adder, list);

	return EXT2_SUCCESS;
}

int fs_rmdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
    EXT2_NODE EXT2Parent;//insert modify
    EXT2_NODE dir;
	int result;
    
    shell_entry_to_ext2_entry(parent, &EXT2Parent);
    
    result = ext2_lookup(&EXT2Parent, name, &dir);
	 if (result)
        return EXT2_ERROR;
    
    return ext2_rmdir(&EXT2Parent , &dir);
}

int fs_remove(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
    EXT2_NODE    EXTParent;
    EXT2_NODE    rmfile;
    int result;
    
    shell_entry_to_ext2_entry(parent, &EXTParent);
    result=ext2_lookup(&EXTParent, name, &rmfile);
    
    if (result)
        return EXT2_ERROR;
    
    return ext2_remove(&EXTParent, &rmfile);
}

/******************************************************************************/
/*																			  */
/*							UTILITY FUNCTIONS 								  */
/*																			  */
/******************************************************************************/
int shell_entry_to_ext2_entry(const SHELL_ENTRY* shell_entry, EXT2_NODE* fat_entry)
{
	EXT2_NODE* entry = (EXT2_NODE*)shell_entry->pdata;

	*fat_entry = *entry;

	return EXT2_SUCCESS;
}

int ext2_entry_to_shell_entry(const EXT2_NODE* ext2_entry, SHELL_ENTRY* shell_entry)
{
	EXT2_NODE* entry = ( EXT2_NODE* )shell_entry->pdata;
    BYTE*    str;
    UINT32 inodeno;
    inodeno=ext2_entry->entry.inode;
    INODE ino;

    get_inode(ext2_entry->fs, inodeno, &ino);
    memset( shell_entry, 0, sizeof( SHELL_ENTRY ) );

    if(inodeno!=2)
    {
        str = shell_entry->name;
        str = my_strncpy( str, ext2_entry->entry.name, 8 );
        if( ext2_entry->entry.name[8] != 0x20 )
        {
            str = my_strncpy( str, ".", 1 );
            str = my_strncpy( str, &ext2_entry->entry.name[8], 3 );
        }
    }
    if(((ino.mode>>12)==(0x4000>>12)) || inodeno==2) 
        shell_entry->isDirectory = 1;
    else
        shell_entry->isDirectory=0;
    
    shell_entry->size=ino.size; 
    
    *entry = *ext2_entry;
    
	return EXT2_SUCCESS;
}

int my_strnicmp(const char* str1, const char* str2, int length)
{
	char   c1, c2;

	while (((*str1 && *str1 != 0x20) || (*str2 && *str2 != 0x20)) && length-- > 0)
	{
		c1 = toupper(*str1);
		c2 = toupper(*str2);

		if (c1 > c2)
			return -1;
		else if (c1 < c2)
			return 1;

		str1++;
		str2++;
	}

	return 0;
}

int is_exist(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
	SHELL_ENTRY_LIST      list;
	SHELL_ENTRY_LIST_ITEM*   current;

	init_entry_list(&list);

	fs_read_dir(disk, fsOprs, parent, &list);
	current = list.first;

	while (current)
	{
		if (my_strnicmp((char*)current->entry.name, name, 12) == 0)
		{
			release_entry_list(&list);
			return EXT2_ERROR;
		}

		current = current->next;
	}
	release_entry_list(&list);
	return EXT2_SUCCESS;
}

char* my_strncpy( char* dest, const char* src, int length )
{
    while( *src && *src != 0x20 && length-- > 0 )
        *dest++ = *src++;
    
    return dest;
}

void printFromP2P(char * start, char * end)
{
	int start_int, end_int;
	start_int = (int)start;
	end_int = (int)end;

	printf("start address : %#x , end address : %#x\n\n", start, end - 1);
	start = (char *)(start_int &= ~(0xf));
	end = (char *)(end_int |= 0xf);

	while (start <= end)
	{
		if ((start_int & 0xf) == 0)
			fprintf(stdout, "\n%#08x   ", start);

		fprintf(stdout, "%02X  ", *(unsigned char *)start);
		start++;
		start_int++;
	}
	printf("\n\n");
}

int adder(void* list, EXT2_NODE* entry)
{
	SHELL_ENTRY_LIST*    entryList = (SHELL_ENTRY_LIST*)list;
    SHELL_ENTRY            newEntry;
    ext2_entry_to_shell_entry(entry, &newEntry);
    add_entry_list(entryList, &newEntry);
    
    return EXT2_SUCCESS;
}

void shell_register_filesystem(SHELL_FILESYSTEM* fs)
{
	*fs = g_fat;
}