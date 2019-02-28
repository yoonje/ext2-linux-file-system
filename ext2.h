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

#define TOTAL_DISK_SIZE			(268435456 + MAX_BLOCK_SIZE) // 256MB + 1KB(BOOT_BLOCK)
#define MAX_SECTOR_SIZE			512 
#define MAX_BLOCK_SIZE			(MAX_SECTOR_SIZE * 2) // 1024

#define MAX_NAME_LENGTH			256
#define MAX_ENTRY_NAME_LENGTH	11

#define EXT2_D_BLOCKS			12
#define EXT2_N_BLOCKS         	15

#define	NUMBER_OF_SECTORS		(TOTAL_DISK_SIZE / MAX_SECTOR_SIZE) // 524290
#define	NUMBER_OF_GROUPS		((TOTAL_DISK_SIZE - MAX_BLOCK_SIZE) / MAX_BLOCK_GROUP_SIZE) // 32
#define	NUMBER_OF_INODES		(NUMBER_OF_BLOCK / 2)  // 131072
#define NUMBER_OF_BLOCK			(TOTAL_DISK_SIZE / MAX_BLOCK_SIZE) // 262145

#define MAX_BLOCK_GROUP_SIZE	(8 * MAX_BLOCK_SIZE * MAX_BLOCK_SIZE) // 8 means 8bits //8388608

#define BLOCKS_PER_GROUP		MAX_BLOCK_GROUP_SIZE / MAX_BLOCK_SIZE // 8192
#define INODES_PER_GROUP        (NUMBER_OF_INODES / NUMBER_OF_GROUPS) // 4096

#define SECTORS_PER_BLOCK		(MAX_BLOCK_SIZE / MAX_SECTOR_SIZE) // 2
#define INODES_PER_BLOCK		(MAX_BLOCK_SIZE / INODE_SIZE) // 8
#define ENTRY_PER_BLOCK			(MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY))

#define GROUP_SIZE     			(BLOCKS_PER_GROUP * MAX_BLOCK_SIZE) // 8388608
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

// group address expressed block(group 0)
#define BOOT_BLOCK_BASE			0 // v1.1.3 M (1 block - boot code) 
#define SUPER_BLOCK_BASE		(BOOT_BLOCK_BASE + 1) // v1.1.3 M  
#define GDT_BASE 				(SUPER_BLOCK_BASE + 1)//BLOCK_NUM_GDT_PER_GROUP) 
#define BLOCK_NUM_GDT_PER_GROUP	(((32*NUMBER_OF_GROUPS) + (MAX_BLOCK_SIZE -1)) / MAX_BLOCK_SIZE) // 나누는 수를 넘지 않는 최대를 수를 더해서 몫을 보존한다.
#define BLOCK_BITMAP_BASE		(GDT_BASE + BLOCK_NUM_GDT_PER_GROUP) // v1.1.3 M        //v 1.1.4 M
#define INODE_BITMAP_BASE		(BLOCK_BITMAP_BASE + 1) // v1.1.3 M 
#define INODE_TABLE_BASE		(INODE_BITMAP_BASE + 1)
#define BLOCK_NUM_IT_PER_GROUP 	(((128*INODES_PER_GROUP) + (MAX_BLOCK_SIZE-1)) / MAX_BLOCK_SIZE) //1.1.4 M // 나누는 수를 넘지 않는 최대를 수를 더해서 몫을 보존한다.
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
	UINT32 max_inode_count; //최대 inode의 개수. 블록의 총 개수를 반으로 계산식을 통해서 정의할 예정
	UINT32 block_count; // 파일 시스템 내의 전체 블록의 개수
	UINT32 reserved_block_count; // 예약영역
	UINT32 free_block_count; // free 블럭 숫자
	UINT32 free_inode_count; // free inode 숫자
	UINT32 first_data_block; // 첫번째 data 블록
	UINT32 log_block_size; // 블록의 크기 지정
	UINT32 log_fragmentation_size; // x
	UINT32 block_per_group; // 각 블록 그룹에 속한 블록의 개수
	UINT32 fragmentation_per_group; // x
	UINT32 inode_per_group; // 각 블록에 속한 inode의 개수
	UINT32 mtime; // x
	UINT32 wtime; // x
	UINT16 mount_count; // x
	UINT16 max_mount_count; // x
	UINT16 magic_signature; // super block인지를 확인하는 고유의 값
	UINT16 state; // 파일 시스템의 상태
	UINT16 errors; // x
	UINT16 minor_version; // xinode.block[number]
	UINT32 last_consistency_check_time; // x
	UINT32 check_interval; // x
	UINT32 creator_os; // 파일시스템을 생성한 운영체제
	UINT16 UID_that_can_use_reserved_blocks; // x
	UINT16 GID_that_can_use_reserved_blocks; // x
	UINT32 first_non_reserved_inode; // 예약되지 않은 첫번째 inode의 인덱스
	UINT16 inode_structure_size; // inode 구조체의 크기
	UINT16 block_group_number; // 현재 super block을 포함하고 있는 블록 그룹의 번호
	UINT32 compatible_feature_flags; // x
	UINT32 incompatible_feature_flags; // x
	UINT32 read_only_feature_flags; // x
	UINT32 UUID[4]; // x
	UINT32 volume_name[4]; // x
	UINT32 last_mounted_path[16]; // x
	UINT32 algorithm_usage_bitmap; // x
	UINT8 preallocated_blocks_count; // x
	UINT8 preallocated_dir_blocks_count; // x
	BYTE padding[2]; // x
	UINT32 journal_UUID[4]; // x
	UINT32 journal_inode_number; // x
	UINT32 journal_device; // x
	UINT32 orphan_inode_list; // x
	UINT32 hash_seed[4]; // x
	UINT8 defined_hash_version; // x
	BYTE padding1; // x
	BYTE padding2[2]; // x
	UINT32 default_mount_option; // x
	UINT32 first_data_block_each_group; // 첫번째 데이터 블록 번호
	BYTE reserved[760]; // x
} EXT2_SUPER_BLOCK;

typedef struct
{
	UINT32 start_block_of_block_bitmap; // 블록 비트맵의 블록 번호. 그룹 디스크립터 테이블 바로 다음에 있는 블록 비트맵의 위치를 의미
	UINT32 start_block_of_inode_bitmap; // indoe 비트맵의 블록 번호. inode 비트맵의 위치를 의미. block bitmap 번호+1과 같음
	UINT32 start_block_of_inode_table; // inode table의 블록 번호. inode 테이블의 위치를 의미
	UINT16 free_blocks_count; // 블록 그룹 내 비어있는 블록의 수
	UINT16 free_inodes_count; // 블록 그룹 내 비어있는 indoe의 수
	UINT16 directories_count; // block의 생성된 디렉토리의 수
	BYTE padding[2]; // x
	BYTE reserved[12]; // x
} EXT2_GROUP_DESCRIPTOR;

typedef struct
{
	UINT16  mode; // indoe의 타입
	UINT16  uid; // x
	UINT32  size; // 파일의 크기
	UINT32  atime; // x       
	UINT32  ctime; // x        
	UINT32  mtime; // x     
	UINT32  dtime; // x  
	UINT16  gid; // x         
	UINT16  links_count; // 링크된 횟수
	UINT32  blocks; // 파일 데이터를 저장하는 블록의 개수
	UINT32  flags; // x
	UINT32  i_reserved1; // x
	UINT32  block[EXT2_N_BLOCKS]; // 데이터 블록을 가르키는 포인터 배열인데 BLOCK 형태의 주소로
	UINT32  generation; // x
	UINT32  file_acl; // x 
	UINT32  dir_acl; // x  
	UINT32  faddr; // x  
	UINT32  i_reserved2[3]; // x 
} INODE;

typedef struct
{
	UINT32 inode; //디렉토리 앤트리의 하위 디렉토리 또는 파일 등을 가르키는 inode
	char name[MAX_ENTRY_NAME_LENGTH]; 
	UINT32 name_len;
	BYTE pad[13];
} EXT2_DIR_ENTRY;

typedef struct
{
    EXT2_GROUP_DESCRIPTOR group_descriptor[NUMBER_OF_GROUPS];
}EXT2_GROUP_DESCRIPTOR_TABLE; // GDT

typedef struct
{
	INODE inode[INODES_PER_GROUP];
}EXT2_INODE_TABLE; // IT

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

//unsigned char toupper(unsigned char ch);
//int isalpha(unsigned char ch);
//int isdigit(unsigned char ch);
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

