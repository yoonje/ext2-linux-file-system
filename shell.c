#pragma warning(disable : 4995)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "shell.h"
#include "disksim.h"

#define		SECTOR					DWORD
#define		BLOCK_SIZE				1024
#define		SECTOR_SIZE				512
#define		NUMBER_OF_SECTORS		((268435456+1024) / SECTOR_SIZE)

#define COND_MOUNT				0x01
#define COND_UMOUNT				0x02

typedef struct
{
	char*	name;
	int(*handler)(int, char**);
	char	conditions;
} COMMAND;

extern void shell_register_filesystem(SHELL_FILESYSTEM*);
extern void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num);
void do_shell(void);
void unknown_command(void);
int seperate_string(char* buf, char* ptrs[]);
double get_percentage(unsigned int number, unsigned int total);

int shell_cmd_format(int argc, char* argv[]);
int shell_cmd_exit(int argc, char* argv[]);
int shell_cmd_mount(int argc, char* argv[]);
int shell_cmd_touch(int argc, char* argv[]);
int shell_cmd_cd(int argc, char* argv[]);
int shell_cmd_ls(int argc, char* argv[]);
int shell_cmd_mkdir(int argc, char* argv[]);
int shell_cmd_fill(int argc, char* argv[]);
int shell_cmd_rm(int argc, char* argv[]);
int shell_cmd_cat(int argc, char* argv[]);
int shell_cmd_rmdir(int argc, char* argv[]);

static COMMAND g_commands[] =
{
	{ "cd",		shell_cmd_cd,		COND_MOUNT	},
	{ "mount",	shell_cmd_mount,	COND_UMOUNT	},
	{ "touch",	shell_cmd_touch,	COND_MOUNT	},
	{ "fill",	shell_cmd_fill,		COND_MOUNT	},
	{ "ls",		shell_cmd_ls,		COND_MOUNT	},
	{ "format",	shell_cmd_format,	COND_UMOUNT	},
	{ "mkdir",	shell_cmd_mkdir,	COND_MOUNT	},
	{ "rm",		shell_cmd_rm,		COND_MOUNT	},
	{ "rmdir",	shell_cmd_rmdir,	COND_MOUNT	},
	{ "exit",	shell_cmd_exit,		0	},
	{ "cat",	shell_cmd_cat,		COND_MOUNT	}
};

static SHELL_FILESYSTEM		g_fs;
static SHELL_FS_OPERATIONS	g_fsOprs;
static SHELL_ENTRY			g_rootDir;
static SHELL_ENTRY			g_currentDir;
static DISK_OPERATIONS		g_disk;

static SHELL_ENTRY	path[256];
static int pathTop = 0;

int g_commandsCount = sizeof(g_commands) / sizeof(COMMAND);
int g_isMounted;

int main(int argc, char* argv[])
{
	if (disksim_init(NUMBER_OF_SECTORS, SECTOR_SIZE, &g_disk) < 0)
	{
		printf("disk simulator initialization has been failed\n");
		return -1;
	}

	shell_register_filesystem(&g_fs);

	do_shell();

	return 0;
}

void do_shell(void)
{
	char buf[1000];
	char command[100];
	char* argv[100];
	int argc;
	int i, j = 0;

	printf("%s File system shell\n", g_fs.name);

	while (-1)
	{
		printf("SSS15NC : [/%s]# ", g_currentDir.name);
		fgets(buf, 1000, stdin);
	
		argc = seperate_string(buf, argv);
		if (argc == 0)
			continue;

		for (i = 0; i < g_commandsCount; i++)
		{
			if (strcmp(g_commands[i].name, argv[0]) == 0)
			{
				if (check_conditions(g_commands[i].conditions) == 0)
					g_commands[i].handler(argc, argv);
				break;
			}
		}
		
		if (argc != 0 && i == g_commandsCount)
			unknown_command();
	}
}
/******************************************************************************/
/*																			  */
/*							SHELL_COMMAND_LIST 						    	  */
/*																			  */
/******************************************************************************/
int shell_cmd_cd(int argc, char* argv[])
{
	SHELL_ENTRY	newEntry;
	int			result, i;

	path[0] = g_rootDir;

	if (argc > 2)
	{
		printf("usage : %s [directory]\n", argv[0]);
		return 0;
	}

	if (argc == 1)
		pathTop = 0;
	else
	{
		if (strcmp(argv[1], ".") == 0)
			return 0;
		else if (strcmp(argv[1], "..") == 0 && pathTop > 0)
			pathTop--;
		else
		{
			result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &newEntry, argv[1]);

			if (result)
			{
				printf("directory not found\n");
				return -1;
			}
			else if (!newEntry.isDirectory)
			{
				printf("%s is not a directory\n", argv[1]);
				return -1;
			}
			path[++pathTop] = newEntry;
		}
	}

	g_currentDir = path[pathTop];

	return 0;
}

int shell_cmd_exit(int argc, char* argv[])
{
	disksim_uninit(&g_disk);
	_exit(0);

	return 0;
}

int shell_cmd_mount(int argc, char* argv[])
{
	int result;
	
	if (g_fs.mount == NULL)
	{
		printf("The mount functions is NULL\n");
		return 0;
	}
	
	result = g_fs.mount(&g_disk, &g_fsOprs, &g_rootDir);
	g_currentDir = g_rootDir;
	
	if (result < 0)
	{
		printf("%s file system mounting has been failed\n", g_fs.name);
		return -1;
	}
	else
	{
		printf("%s file system has been mounted successfully\n", g_fs.name);
		g_isMounted = 1;
	}

	return 0;
}

int shell_cmd_umount(int argc, char* argv[])
{
	g_isMounted = 0;

	if (g_fs.umount == NULL)
		return 0;

	g_fs.umount(&g_disk, &g_fsOprs);
	return 0;
}

int shell_cmd_touch(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int			result;
	if (argc < 2)
	{
		printf("usage : touch [files...]\n");
		return -2;
	}
	result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);
	
	if (result)
	{
		printf("create failed\n");
		return -1;
	}

	return 0;
}

int shell_cmd_fill(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	char*		buffer;
	char*		tmp;
	int			size;
	int			result;

	if( argc != 3 ) 
	{
		printf( "usage : fill [file] [size]\n" );
		return 0;
	}

	sscanf( argv[2], "%d", &size ); 

	result = g_fsOprs.fileOprs->create( &g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry ); 
	
	if( result ) 
	{
		printf( "create failed\n" );
		return -1;
	}

	buffer = ( char* )malloc( size + 13 ); 
	tmp = buffer; 

	while( tmp < buffer + size ) 
	{
		memcpy( tmp, "Can you see? ", 13 );
		tmp += 13;
	}

	g_fsOprs.fileOprs->write( &g_disk, &g_fsOprs, &g_currentDir, &entry, 0, size, buffer );
	
	free( buffer );

	return 0;
}

int shell_cmd_rm(int argc, char* argv[])
{
	int i;

	if (argc < 2)
	{
		printf("usage : rm [files...]\n");
		return 0;
	}

	for (i = 1; i < argc; i++)
	{
		if (g_fsOprs.fileOprs->remove(&g_disk, &g_fsOprs, &g_currentDir, argv[i]))
			printf("cannot remove file\n");
	}

	return 0;
}

int shell_cmd_format(int argc, char* argv[])
{
	int		result;
	unsigned int k = 0;
	char*	param = NULL;

	if (argc >= 2)
		param = argv[1];

	result = g_fs.format(&g_disk);

	if (result < 0)
	{
		printf("%s formatting is failed\n", g_fs.name);
		return -1;
	}

	printf("disk has been formatted successfully\n");
	return 0;
}

int shell_cmd_df(int argc, char* argv[])
{
	unsigned int used, total;
	int result;

	g_fsOprs.stat(&g_disk, &g_fsOprs, &total, &used);

	printf("free sectors : %u(%.2lf%%)\tused sectors : %u(%.2lf%%)\ttotal : %u\n",
		total - used, get_percentage(total - used, g_disk.numberOfSectors),
		used, get_percentage(used, g_disk.numberOfSectors),
		total);

	return 0;
}

int shell_cmd_mkdir(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int result;

	if (argc != 2)
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if (result)
	{
		printf("cannot create directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_rmdir(int argc, char* argv[])
{
	int result;

	if (argc != 2)
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.rmdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1]);

	if (result)
	{
		printf("cannot remove directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_mkdirst(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int		result, i, count;
	char	buf[10];

	if (argc != 2)
	{
		printf("usage : %s [count]\n", argv[0]);
		return 0;
	}

	sscanf(argv[1], "%d", &count);
	for (i = 0; i < count; i++)
	{
		printf(buf, "%d", i);
		result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, buf, &entry);

		if (result)
		{
			printf("cannot create directory\n");
			return -1;
		}
	}

	return 0;
}

int shell_cmd_cat(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	char		buf[1025] = { 0, };
	int			result;
	unsigned long	offset = 0;

	if (argc != 2)
	{
		printf("usage : %s [file name]\n", argv[0]);
		return 0;
	}

	result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1]);
	if (result)
	{
		printf("%s lookup failed\n", argv[1]);
		return -1;
	}

	while ((result = (g_fsOprs.fileOprs->read(&g_disk, &g_fsOprs, &g_currentDir, &entry, offset, 1024, buf))) > 0)
	{
		printf("%s", buf);
		offset += 1024;
		memset(buf, 0, sizeof(buf));
	}
	printf("\n");
}

int shell_cmd_ls(int argc, char* argv[])
{
	SHELL_ENTRY_LIST		list;
	SHELL_ENTRY_LIST_ITEM*	current;

	if (argc > 2)
	{
		printf("usage : %s [path]\n", argv[0]);
		return 0;
	}
	init_entry_list(&list);
	if (g_fsOprs.read_dir(&g_disk, &g_fsOprs, &g_currentDir, &list))
	{
		printf("Failed to read_dir\n");
		return -1;
	}

	current = list.first;

	printf("[File names] [D] [File sizes]\n");
	while (current)
	{
		printf("%-12s  %1d  %12d\n",
			current->entry.name, current->entry.isDirectory, current->entry.size);
		current = current->next;
	}
	printf("\n");

	release_entry_list(&list);
	return 0;
}

/******************************************************************************/
/*																			  */
/*							UTILITY FUNCTIONS 								  */
/*																			  */
/******************************************************************************/

int check_conditions(int conditions)
{
	if (conditions & COND_MOUNT && !g_isMounted)
	{
		printf("file system is not mounted\n");
		return -1;
	}

	if (conditions & COND_UMOUNT && g_isMounted)
	{
		printf("file system is already mounted\n");
		return -1;
	}

	return 0;
}

void unknown_command(void)
{
	int i;

	printf(" * ");
	for (i = 0; i < g_commandsCount; i++)
	{
		if (i < g_commandsCount - 1)
			printf("%s, ", g_commands[i].name);
		else
			printf("%s", g_commands[i].name);
	}
	printf("\n");
}

int seperate_string(char* buf, char* ptrs[])
{
	char prev = 0;
	int count = 0;

	while (*buf)
	{
		if (isspace(*buf))
			*buf = 0;
		else if (prev == 0)
			ptrs[count++] = buf;

		prev = *buf++;
	}

	return count;
}

double get_percentage(unsigned int number, unsigned int total)
{
	return ((double)number) / total * 100.;
}