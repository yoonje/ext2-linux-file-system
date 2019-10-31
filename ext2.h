#ifndef _FAT_H_
#define _FAT_H_

#include "common.h"
#include "disk.h"
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define SHELL_BUFFER_SIZE		13

#define TOTAL_DISK_SIZE			(268435456 + MAX_BLOCK_SIZE)
#define MAX_SECTOR_SIZE			512 
#define MAX_BLOCK_SIZE			(MAX_SECTOR_SIZE * 2)

#define MAX_NAME_LENGTH			256
#define MAX_ENTRY_NAME_LENGTH	11

#define EXT2_D_BLOCKS			12
#define EXT2_N_BLOCKS         	15

#define	NUMBER_OF_SECTORS		(TOTAL_DISK_SIZE / MAX_SECTOR_SIZE)
#define	NUMBER_OF_GROUPS		((TOTAL_DISK_SIZE - MAX_BLOCK_SIZE) / MAX_BLOCK_GROUP_SIZE)
#define	NUMBER_OF_INODES		(NUMBER_OF_BLOCK / 2)
#define NUMBER_OF_BLOCK			(TOTAL_DISK_SIZE / MAX_BLOCK_SIZE)

#define MAX_BLOCK_GROUP_SIZE	(8 * MAX_BLOCK_SIZE * MAX_BLOCK_SIZE)

#define BLOCKS_PER_GROUP		MAX_BLOCK_GROUP_SIZE / MAX_BLOCK_SIZE
#define INODES_PER_GROUP        (NUMBER_OF_INODES / NUMBER_OF_GROUPS)

#define SECTORS_PER_BLOCK		(MAX_BLOCK_SIZE / MAX_SECTOR_SIZE)
#define INODES_PER_BLOCK		(MAX_BLOCK_SIZE / INODE_SIZE)
#define ENTRY_PER_BLOCK			(MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY))

#define GROUP_SIZE     			(BLOCKS_PER_GROUP * MAX_BLOCK_SIZE)
#define INODE_SIZE			    128 

#define USER_PERMITION 			0x1FF 
#define DIRECTORY_TYPE			0x4000
#define FILE_TYPE				0x2000

#define	VOLUME_LABLE			"EXT2 BY SSS15NC"

#define ATTR_READ_ONLY			0x01
#define ATTR_HIDDEN				0x02
#define ATTR_SYSTEM				0x04
#define ATTR_VOLUME_ID			0x08
#define ATTR_DIRECTORY			0x10
#define ATTR_ARCHIVE			0x20
#define ATTR_LONG_NAME			ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

#define DIR_ENTRY_FREE			0x01
#define DIR_ENTRY_NO_MORE		0x00
#define DIR_ENTRY_OVERWRITE		1

#define RESERVED_INODE_NUM		11
#define NUM_OF_ROOT_INODE		2

#define SPACE_CHAR				0x20


#define BOOT_BLOCK_BASE			0
#define SUPER_BLOCK_BASE		(BOOT_BLOCK_BASE + 1)
#define GDT_BASE 				(SUPER_BLOCK_BASE + 1)
#define BLOCK_NUM_GDT_PER_GROUP	(((32*NUMBER_OF_GROUPS) + (MAX_BLOCK_SIZE -1)) / MAX_BLOCK_SIZE)
#define BLOCK_BITMAP_BASE		(GDT_BASE + BLOCK_NUM_GDT_PER_GROUP)
#define INODE_BITMAP_BASE		(BLOCK_BITMAP_BASE + 1)
#define INODE_TABLE_BASE		(INODE_BITMAP_BASE + 1)
#define BLOCK_NUM_IT_PER_GROUP 	(((128*INODES_PER_GROUP) + (MAX_BLOCK_SIZE-1)) / MAX_BLOCK_SIZE)
#define DATA_BLOCK_BASE			(INODE_TABLE_BASE + BLOCK_NUM_IT_PER_GROUP)

#define GET_INODE_GROUP(x) 		((x) - 1)/( NUMBER_OF_INODES / NUMBER_OF_GROUPS )
#define GET_DATA_GROUP(x)		((x) - 1)/(BLOCKS_PER_GROUP)
#define SQRT(x)  				( (x) * (x)  )
#define TRI_SQRT(x)  			( (x) * (x) * (x) )
#define WHICH_GROUP_BLONG(x) 	( ( (x) - 1)  / ( NUMBER_OF_INODES / NUMBER_OF_GROUPS )  )

#define TSQRT(x) 				((x)*(x)*(x))
#define GET_INODE_FROM_NODE(x) ((x)->entry.inode)

#ifdef _WIN32
#pragma pack(push,fatstructures)
#endif
#pragma pack(1)

typedef struct
{
	UINT32 max_inode_count;
	UINT32 block_count;
	UINT32 reserved_block_count;
	UINT32 free_block_count;
	UINT32 free_inode_count;
	UINT32 first_data_block;
	UINT32 log_block_size;
	UINT32 log_fragmentation_size;
	UINT32 block_per_group;
	UINT32 fragmentation_per_group;
	UINT32 inode_per_group;
	UINT32 mtime;
	UINT32 wtime;
	UINT16 mount_count;
	UINT16 max_mount_count;
	UINT16 magic_signature;
	UINT16 state;
	UINT16 errors;
	UINT16 minor_version;
	UINT32 last_consistency_check_time;
	UINT32 check_interval;
	UINT32 creator_os;
	UINT16 UID_that_can_use_reserved_blocks;
	UINT16 GID_that_can_use_reserved_blocks;
	UINT32 first_non_reserved_inode;
	UINT16 inode_structure_size;
	UINT16 block_group_number;
	UINT32 compatible_feature_flags;
	UINT32 incompatible_feature_flags;
	UINT32 read_only_feature_flags;
	UINT32 UUID[4];
	UINT32 volume_name[4];
	UINT32 last_mounted_path[16];
	UINT32 algorithm_usage_bitmap;
	UINT8 preallocated_blocks_count;
	UINT8 preallocated_dir_blocks_count;
	BYTE padding[2];
	UINT32 journal_UUID[4];
	UINT32 journal_inode_number;
	UINT32 journal_device;
	UINT32 orphan_inode_list;
	UINT32 hash_seed[4];
	UINT8 defined_hash_version;
	BYTE padding1;
	BYTE padding2[2];
	UINT32 default_mount_option;
	UINT32 first_data_block_each_group;
	BYTE reserved[760];
} EXT2_SUPER_BLOCK;

typedef struct
{
	UINT32 start_block_of_block_bitmap;
	UINT32 start_block_of_inode_bitmap;
	UINT32 start_block_of_inode_table;
	UINT16 free_blocks_count;
	UINT16 free_inodes_count;
	UINT16 directories_count;
	BYTE padding[2];
	BYTE reserved[12];
} EXT2_GROUP_DESCRIPTOR;

typedef struct
{
	UINT16  mode;
	UINT16  uid;
	UINT32  size;
	UINT32  atime;
	UINT32  ctime;
	UINT32  mtime;
	UINT32  dtime;
	UINT16  gid;
	UINT16  links_count;
	UINT32  blocks;
	UINT32  flags;
	UINT32  i_reserved1;
	UINT32  block[EXT2_N_BLOCKS];
	UINT32  generation;
	UINT32  file_acl;
	UINT32  dir_acl;
	UINT32  faddr;
	UINT32  i_reserved2[3];
} INODE;

typedef struct
{
	UINT32 inode;
	char name[MAX_ENTRY_NAME_LENGTH]; 
	UINT32 name_len;
	BYTE pad[13];
} EXT2_DIR_ENTRY;

typedef struct
{
    EXT2_GROUP_DESCRIPTOR group_descriptor[NUMBER_OF_GROUPS];
}EXT2_GROUP_DESCRIPTOR_TABLE;

typedef struct
{
	INODE inode[INODES_PER_GROUP];
}EXT2_INODE_TABLE;

typedef struct
{
	BYTE block_bitmap[MAX_BLOCK_SIZE]; 
}EXT2_BLOCK_BITMAP;

typedef struct
{
	BYTE inode_bitmap[MAX_BLOCK_SIZE]; 
}EXT2_INODE_BITMAP;

typedef struct
{
	EXT2_SUPER_BLOCK		sb;
	EXT2_GROUP_DESCRIPTOR	gd;
	DISK_OPERATIONS*		disk;
} EXT2_FILESYSTEM;

typedef struct
{
	UINT32 group;
	UINT32 block;
	UINT32 offset;
} EXT2_DIR_ENTRY_LOCATION;

typedef struct
{
	EXT2_FILESYSTEM * fs;
	EXT2_DIR_ENTRY entry;
	EXT2_DIR_ENTRY_LOCATION location;
} EXT2_NODE;

typedef struct
{
    int bit_num;
    int index_num;
	int aa_num
}EXT2_BITSET;

#ifdef _WIN32
#pragma pack(pop, fatstructures)
#else
#pragma pack()
#endif

#define SUPER_BLOCK 0
#define GROUP_DES  1
#define BLOCK_BITMAP 2
#define INODE_BITMAP 3
#define INODE_TABLE(x) (4 + x)

#define FILE_TYPE_FIFO               0x1000
#define FILE_TYPE_CHARACTERDEVICE    0x2000
#define FILE_TYPE_DIR				 0x4000
#define FILE_TYPE_BLOCKDEVICE        0x6000
#define FILE_TYPE_FILE				 0x8000

int ext2_format(DISK_OPERATIONS* disk);
int ext2_create(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry);
int ext2_lookup(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry);

int fill_super_block(EXT2_SUPER_BLOCK * sb);
int fill_descriptor_table(EXT2_GROUP_DESCRIPTOR_TABLE * gd, EXT2_SUPER_BLOCK * sb);

int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb);

typedef int(*EXT2_NODE_ADD)(void*, EXT2_NODE*);

int block_write(DISK_OPERATIONS* this, SECTOR sector, void* data);
int block_read(DISK_OPERATIONS* this, SECTOR sector, void* data);

UINT32 get_free_inode_number(EXT2_FILESYSTEM* fs);
UINT32 get_free_block_number(EXT2_FILESYSTEM* fs);

int init_super_block(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, UINT32 group_number);
int init_gdt(DISK_OPERATIONS* disk, EXT2_GROUP_DESCRIPTOR_TABLE * gdt, UINT32 group_number);
int init_block_bitmap(DISK_OPERATIONS* disk, UINT32 group_number);
int init_inode_bitmap(DISK_OPERATIONS* disk, UINT32 group_number);

int set_bit(SECTOR number, BYTE* bitmap);
int dump_memory(DISK_OPERATIONS* disk, int sector);
int validate_sb(void* block);
int get_inode(EXT2_FILESYSTEM* fs, UINT32 inode_num, INODE *inodeBuffer);
int read_root_block(EXT2_FILESYSTEM* fs, BYTE* block);
int read_superblock(EXT2_FILESYSTEM* fs, EXT2_NODE* root);
int format_name(EXT2_FILESYSTEM* fs, char* name);
int lookup_entry(EXT2_FILESYSTEM* fs, int inode, char* name, EXT2_NODE* retEntry);
int find_entry_at_block(BYTE* sector, char* formattedName, UINT32 begin, UINT32 last, UINT32* number);

int set_inode_bitmap(EXT2_FILESYSTEM* fs, EXT2_INODE_BITMAP* i_bitmap, EXT2_BITSET bitset);
int set_block_bitmap(EXT2_FILESYSTEM* fs, EXT2_BLOCK_BITMAP* b_bitmap, EXT2_BITSET bitset);
int insert_inode_table(EXT2_NODE* parent, INODE* inode_entry, int free_inode);

int set_sb_free_block_count(EXT2_NODE* NODE, int num);
int	set_sb_free_inode_count(EXT2_NODE* NODE, int num);

int	set_gd_free_block_count(EXT2_NODE* NODE,UINT32 free_data_block,int num);
int	set_gd_free_inode_count(EXT2_NODE* NODE,UINT32 free_inode_block,int num);

int set_entry( EXT2_FILESYSTEM* fs,EXT2_DIR_ENTRY_LOCATION* location, EXT2_DIR_ENTRY* value );
int insert_entry(EXT2_NODE *, EXT2_NODE *, UINT16 );

#endif

