typedef struct
{
	char*	address;
} DISK_MEMORY;

#include "ext2.h"
#include "disksim.h"
#define MIN( a, b )					( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )					( ( a ) > ( b ) ? ( a ) : ( b ) )


/******************************************************************************/
/*																			  */
/*								EXT2_COMMAND 								  */
/*																			  */
/******************************************************************************/
int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, char* buffer)
{
	BYTE block[MAX_BLOCK_SIZE];
	INODE nodeBuffer;
	int i;
	EXT2_DIR_ENTRY * dir;
	EXT2_DIR_ENTRY my_entry;
	INT32 block_num;
	DISK_OPERATIONS* disk = file->fs->disk;
	INODE curr_inode;

	block_num = length / MAX_BLOCK_SIZE;
	block_read(disk, file->location.block, block);
	dir=(EXT2_DIR_ENTRY*)block;
	my_entry = dir[file->location.offset];
	
	get_inode(file->fs, my_entry.inode, &curr_inode);
	curr_inode.size = length;

	if(block_num < EXT2_D_BLOCKS){ // only direct area
		curr_inode.blocks = block_num;
        for(i=0; i < block_num-1; i++)
        {
            curr_inode.block[i+1] = get_free_block_number(file->fs);
        }
	}
	else{
		return EXT2_ERROR; // indirect area
	}

	fill_data_block(disk, &curr_inode, length, offset, buffer, block_num);
	insert_inode_table(file, &curr_inode, my_entry.inode);

	return EXT2_SUCCESS;
}


int ext2_read(EXT2_NODE * EXT2Entry, int offset, unsigned long length, char* buffer )
{
    BYTE block[MAX_BLOCK_SIZE];
    int count = offset / MAX_BLOCK_SIZE; // looping count
    INODE inodeBuffer;

    get_inode(EXT2Entry->fs, EXT2Entry->entry.inode, &inodeBuffer); // inode 번호를 구한다 
    block_read(EXT2Entry->fs->disk, inodeBuffer.block[count], block); // inodeBuffer를 통해서 값을 입력 받는다
    memcpy(buffer, block, MAX_BLOCK_SIZE-1); // 복사해준다
    
    if(inodeBuffer.blocks + 1 > count) // 더 읽어올지 그만 읽어올지를 정해주는 부분 - shell.c와 연동
        return 1; // more read
    return -1; // no more read
}

int ext2_rmdir( EXT2_NODE* parent,EXT2_NODE* rmdir ){//insert modify
	DISK_OPERATIONS* disk= parent->fs->disk;
	INODE inodeBuffer;
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_DIR_ENTRY* dir;
	INT32 count=1;//지우므로 free갯수는 증가
	INT32 inodenum,groupnum;
	char* entryName=NULL;
	EXT2_NODE bufferNode;
	
	rmdir->fs = parent->fs;
	int i,bitmap_num;
	get_inode(parent->fs,rmdir->entry.inode,&inodeBuffer);
	//type..
    if(!(inodeBuffer.mode & FILE_TYPE_DIR)) {
		printf("this is not type DIR!!\n");
		return EXT2_ERROR;
	}

	//if find anything... it is error.
	if(lookup_entry(parent->fs, rmdir->entry.inode,entryName,&bufferNode)==EXT2_SUCCESS) 
			return EXT2_ERROR;

	//directory안에 아무것도 없는 것이 확인 되었을 때 수행..

	//사용 블럭 제로화
	for(i=0;i<inodeBuffer.blocks;i++){
		ZeroMemory(block,MAX_BLOCK_SIZE);
		if(i <= 12){
		block_write(disk,inodeBuffer.block[i],block);
		set_gd_free_block_count(rmdir,inodeBuffer.block[i],count);
		set_sb_free_block_count(rmdir,count);
		
		//set block bit
		block_read(disk,BLOCKS_PER_GROUP*GET_DATA_GROUP(inodeBuffer.block[i])+BLOCK_BITMAP_BASE,block);
		bitmap_num = (inodeBuffer.block[i] % (BLOCKS_PER_GROUP) ) - DATA_BLOCK_BASE;
		set_zero_bit(bitmap_num,block);//----------------------------
		
		block_read(disk,BLOCKS_PER_GROUP*GET_DATA_GROUP(inodeBuffer.block[i])+BLOCK_BITMAP_BASE,block);
		}
		else{
			//indirect}
			}
	}

	// delete file location..
	block_read(disk,rmdir->location.block,block);
    dir = (EXT2_DIR_ENTRY*)block;
    dir += rmdir->location.offset;

	inodenum = dir->inode; //지우므로 미리 저장 해둠
	groupnum = inodenum/INODES_PER_GROUP;

	//delete entry in parent block & make free mode
	ZeroMemory(dir,sizeof(EXT2_DIR_ENTRY));
	block_write(disk,rmdir->location.block,block);
	dir->name[0]=DIR_ENTRY_FREE;//DIR_ENTRY_FREE;
	set_entry(parent->fs,&rmdir->location,dir);


	//delete file's-->inode_table delete, inode bitmap관리
	ZeroMemory(&inodeBuffer,sizeof(INODE));
	insert_inode_table(parent, &inodeBuffer, inodenum);

	
	block_read(disk,groupnum*BLOCKS_PER_GROUP+INODE_BITMAP_BASE,block);
	bitmap_num = (inodenum%INODES_PER_GROUP)-1;
	set_zero_bit(bitmap_num,block); //inode가 1부터 시작하므로 -1을 해준다.
	block_write(disk,groupnum*BLOCKS_PER_GROUP+INODE_BITMAP_BASE,block);

	//free inode count 관리
	set_sb_free_inode_count(rmdir,count);
	set_gd_free_inode_count(rmdir,inodenum,count);
	
    return EXT2_SUCCESS;
}

int ext2_remove(EXT2_NODE* parent, EXT2_NODE* rmfile)
{
    DISK_OPERATIONS* disk = parent->fs->disk;
	INODE inodeBuffer;
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_DIR_ENTRY* dir;
	INT32 count = 1; // 지우므로 free 개수는 증가
	INT32 inodenum,groupnum;

	rmfile->fs = parent->fs;
	INT32 i,bitmap_num;

	get_inode(parent->fs, rmfile->entry.inode, &inodeBuffer);
	
	// type exception handler 
    if(inodeBuffer.mode & FILE_TYPE_DIR){
		printf("type DIR!!\n");
		return EXT2_ERROR;
	}
	
	// 사용 블럭 제로화
	for(i=0 ; i<inodeBuffer.blocks; i++){
		ZeroMemory(block, MAX_BLOCK_SIZE);
		if(i <= EXT2_D_BLOCKS){ // direct
		block_write(disk, inodeBuffer.block[i], block);
		set_gd_free_block_count(rmfile, inodeBuffer.block[i], count);
		set_sb_free_block_count(rmfile, count);

		// setting block bit
		block_read(disk, BLOCKS_PER_GROUP*GET_DATA_GROUP(inodeBuffer.block[i]) + BLOCK_BITMAP_BASE, block);
		bitmap_num = (inodeBuffer.block[i] % (BLOCKS_PER_GROUP) ) - DATA_BLOCK_BASE;
		set_zero_bit(bitmap_num, block);		
		block_read(disk,BLOCKS_PER_GROUP*GET_DATA_GROUP(inodeBuffer.block[i])+BLOCK_BITMAP_BASE,block);
		}
		else{
			// indirect area
			}
	}

	// delete file location..
	block_read(disk,rmfile->location.block,block);
    dir = (EXT2_DIR_ENTRY*)block;
    dir += rmfile->location.offset;

	inodenum = dir->inode; // 지우므로 미리 저장 해둠
	groupnum = inodenum/INODES_PER_GROUP;

	//delete entry in parent block & make free mode
	ZeroMemory(dir, sizeof(EXT2_DIR_ENTRY));
	block_write(disk, rmfile->location.block, block);
	dir->name[0] = DIR_ENTRY_FREE;
	set_entry(parent->fs, &rmfile->location, dir);

	//delete file's-->inode_table delete, inode bitmap관리
	ZeroMemory(&inodeBuffer, sizeof(INODE));
	insert_inode_table(parent, &inodeBuffer, inodenum);

	block_read(disk,groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);
	bitmap_num = (inodenum % INODES_PER_GROUP)-1;
	set_zero_bit(bitmap_num, block); //inode가 1부터 시작하므로 -1을 해준다.
	block_write(disk, groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);

	//free inode count 관리
	set_sb_free_inode_count(rmfile, count);
	set_gd_free_inode_count(rmfile, inodenum, count);
	
    return EXT2_SUCCESS;
}

int ext2_mkdir(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry)
{
	EXT2_NODE      dotNode, dotdotNode;
	DWORD         firstCluster;
	char         name[MAX_ENTRY_NAME_LENGTH];
	int            result;
	int            i;
	int free_inode,free_data_block,count;
	INODE inode_entry;
	
	strcpy((char*)name, entryName);

	format_name(parent->fs, (char*)name);

	free_inode = get_free_inode_number(parent->fs); // free_inode는 결국 현재 생성하는 것의 아이노드 번호가 된다.
	free_data_block = get_free_block_number(parent->fs); // 할당할 data block 찾기

	count = -1;
	retEntry->fs = parent->fs;
	set_sb_free_block_count(retEntry,count);
	set_sb_free_inode_count(retEntry,count);
	
	set_gd_free_block_count(retEntry,free_data_block,count);
	set_gd_free_inode_count(retEntry,free_inode,count);

	ZeroMemory(retEntry, sizeof(EXT2_NODE));
	memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH); // 새로 만들 디렉토리정보 넣어줌
	retEntry->entry.name_len = strlen((char*)retEntry->entry.name);
	retEntry->entry.inode = free_inode;
	retEntry->fs = parent->fs;
	result = insert_entry(parent, retEntry, FILE_TYPE_DIR);
	
	if (result == EXT2_ERROR)
		return EXT2_ERROR;
	
	ZeroMemory(&inode_entry, sizeof(INODE)); // inode entry 할당
    inode_entry.block[0] = free_data_block; // free data block 할당
    inode_entry.mode = USER_PERMITION | FILE_TYPE_DIR; // 파일 mode
	inode_entry.blocks = 1;
	inode_entry.size=0;   // inode table을 최신화 시켜주기위한코드
	
	insert_inode_table(parent, &inode_entry, free_inode); // inode insert
	
	ZeroMemory(&dotNode, sizeof(EXT2_NODE));
	memset(dotNode.entry.name, 0x20, 11);
	
	dotNode.entry.name[0] = '.';
	dotNode.fs = retEntry->fs;
	dotNode.entry.inode = retEntry->entry.inode;
	insert_entry(retEntry, &dotNode, FILE_TYPE_DIR);
	
	
	ZeroMemory(&dotdotNode, sizeof(EXT2_NODE));
	memset(dotdotNode.entry.name, 0x20, 11);
	dotdotNode.entry.name[0] = '.';
	dotdotNode.entry.name[1] = '.';
	dotdotNode.entry.inode = parent->entry.inode;
	dotdotNode.fs = retEntry->fs;
	insert_entry(retEntry, &dotdotNode, FILE_TYPE_DIR);
	
	return EXT2_SUCCESS;
}

int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list)
{
	BYTE   block[MAX_BLOCK_SIZE]; // 블럭 사이즈 할당 
	INODE* inodeBuffer; // INODE 버퍼
	int i, result, block_num;
	UINT32 group_num;

	group_num = dir->fs->sb.block_group_number;
	inodeBuffer = (INODE*)malloc(sizeof(INODE));

	ZeroMemory(block, MAX_BLOCK_SIZE);
	ZeroMemory(inodeBuffer, sizeof(INODE));
	result = get_inode(dir->fs, dir->entry.inode, inodeBuffer);

	if (result == EXT2_ERROR)
		return EXT2_ERROR;

	for (i = 0; i < inodeBuffer->blocks; ++i){
		block_num = get_data_block_at_inode(dir->fs, *inodeBuffer, i); // 데이터 블럭의 번호가 넘어옴
		block_read(dir->fs->disk, block_num, block);
		read_dir_from_block(dir->fs, block, adder, list);
	}
	return EXT2_SUCCESS;
}

int ext2_format(DISK_OPERATIONS* disk)
{
	EXT2_SUPER_BLOCK sb;
    EXT2_GROUP_DESCRIPTOR_TABLE gdt;
    EXT2_INODE_BITMAP ib;
    EXT2_BLOCK_BITMAP bb;
	EXT2_INODE_TABLE it;
	int i;

	if (fill_super_block(&sb) != EXT2_SUCCESS)
	 			return EXT2_ERROR;
	if (fill_descriptor_table(&gdt,&sb) != EXT2_SUCCESS)
	 		 	return EXT2_ERROR;
	init_inode_bitmap(disk, i);

	for (i = 0; i < NUMBER_OF_GROUPS; i++)
    {
			sb.block_group_number = i;
	 		init_super_block(disk, &sb, i);
	 		init_gdt(disk, &gdt, i);
    		init_block_bitmap(disk, i);
	//		init_data_block(disk, i); ---> diskim.c의 disk_init에서 0으로 초기화 해주기 때문에 해주는 의미가 없다.
	}
	PRINTF("max inode count                : %d\n", sb.max_inode_count);
	PRINTF("total block count              : %u\n", sb.block_count);
	PRINTF("byte size of inode structure   : %u\n", sb.inode_structure_size);
	PRINTF("block byte size                : %u\n", MAX_BLOCK_SIZE);
	PRINTF("total sectors count            : %u\n", NUMBER_OF_SECTORS);
	PRINTF("sector byte size               : %u\n", MAX_SECTOR_SIZE);
	PRINTF("\n");
	create_root(disk, &sb);
	return EXT2_SUCCESS;
}

int ext2_lookup(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry)
{
	EXT2_DIR_ENTRY_LOCATION	begin;
	char formattedName[MAX_NAME_LENGTH] = { 0, };
	
	strncpy(formattedName, entryName, MAX_ENTRY_NAME_LENGTH);
	format_name(parent->fs, formattedName);
	
	int result=lookup_entry(parent->fs, parent->entry.inode, formattedName, retEntry); // 같은 이름이 있는지 or 꽉찼는지 점검한다
	
	return result;
}

int ext2_create(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry)
{
	EXT2_DIR_ENTRY dir_entry;
	UINT32 inode;
	char name[MAX_ENTRY_NAME_LENGTH] = { 0, };
	BYTE block[MAX_BLOCK_SIZE];
	INODE inode_entry;
	UINT32 result;
	UINT32 free_inode;
	UINT32 free_data_block;
	INT32 count;
	
	if ((parent->fs->gd.free_inodes_count) == 0) return EXT2_ERROR;	// free indoe가 없으면 에러

	strncpy(name, entryName, MAX_ENTRY_NAME_LENGTH);

	if (format_name(parent->fs, name) == EXT2_ERROR) return EXT2_ERROR; //이름을 파싱
	
	ZeroMemory(retEntry, sizeof(EXT2_NODE)); // initialize
	memcpy(retEntry->entry.name, entryName, MAX_ENTRY_NAME_LENGTH); // 파싱된 네임을 ext2_entry에 저장
	
	if (ext2_lookup(parent, name, retEntry) == EXT2_SUCCESS) return EXT2_ERROR;
	
	retEntry->fs = parent->fs; // 파일 시스템 적용

	free_inode = get_free_inode_number(parent->fs); // free_inode는 결국 현재 생성하는 것의 아이노드 번호가 된다.
	free_data_block = get_free_block_number(parent->fs); // 할당할 data block 찾기
	
	count = -1;
	set_sb_free_block_count(retEntry,count);
	set_sb_free_inode_count(retEntry,count);

	set_gd_free_block_count(retEntry,free_data_block,count);
	set_gd_free_inode_count(retEntry,free_inode,count);

	ZeroMemory(&inode_entry, sizeof(INODE)); // inode entry 할당
    inode_entry.block[0] = free_data_block; // free data block 할당
    inode_entry.mode = USER_PERMITION | 0x2000; // 파일 mode
	inode_entry.blocks = 1;
	inode_entry.size=0;   //inode table을 최신화 시켜주기위한코드
	insert_inode_table(parent, &inode_entry, free_inode); // inode insert

	ZeroMemory(&dir_entry, sizeof(EXT2_DIR_ENTRY)); // dir entry 할당
    memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH); // 이름 할당
    retEntry->entry.name_len = my_strnlen(retEntry->entry.name);
	retEntry->entry.inode = free_inode; // inode 할당      

	insert_entry(parent, retEntry , inode_entry.mode);
	 
	return EXT2_SUCCESS;
}


/******************************************************************************/
/*																			  */
/*								init function 								  */
/*																			  */
/******************************************************************************/
int fill_super_block(EXT2_SUPER_BLOCK * sb)
{
	ZeroMemory(sb, sizeof(EXT2_SUPER_BLOCK));
	// 주의! 파일 할당/삭제 할 때 superblock 조정해줘야함 check 표시해둠
	sb->max_inode_count = NUMBER_OF_INODES;
	sb->first_data_block_each_group = 1 + BLOCK_NUM_GDT_PER_GROUP + 1 + 1 + ((INODES_PER_GROUP*INODE_SIZE + (MAX_BLOCK_SIZE - 1))/MAX_BLOCK_SIZE); // 1.1.3 M 수정 필요
	sb->block_count = NUMBER_OF_BLOCK;
	sb->reserved_block_count = sb->block_count/100 * 5;
	sb->free_block_count = NUMBER_OF_BLOCK - (sb->first_data_block_each_group) - 1; // check
	sb->free_inode_count = NUMBER_OF_INODES - 11; // check 
	sb->first_data_block = SUPER_BLOCK_BASE; // 수정 필요
	sb->log_block_size = 0; // 0이 1KB를 의미
	sb->log_fragmentation_size = 0;
	sb->block_per_group = BLOCKS_PER_GROUP;
	sb->fragmentation_per_group = 0;
	sb->inode_per_group = NUMBER_OF_INODES / NUMBER_OF_GROUPS;
	sb->magic_signature = 0xEF53;
	sb->errors = 0;
	sb->first_non_reserved_inode = 11;  
	sb->inode_structure_size = 128;
	
	return EXT2_SUCCESS;
}

int fill_descriptor_table(EXT2_GROUP_DESCRIPTOR_TABLE * gdb, EXT2_SUPER_BLOCK * sb)
{
	EXT2_GROUP_DESCRIPTOR gd;
	int i;

	ZeroMemory(gdb, sizeof(EXT2_GROUP_DESCRIPTOR_TABLE));
	ZeroMemory(&gd, sizeof(EXT2_GROUP_DESCRIPTOR));
   
	for (i = 0; i < NUMBER_OF_GROUPS; i++)
    {
		gd.start_block_of_block_bitmap = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP*i);
		gd.start_block_of_inode_bitmap = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP*i)+1; 
		gd.start_block_of_inode_table = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP*i)+1+1; 
		gd.free_blocks_count = (sb->free_block_count / NUMBER_OF_GROUPS); // check
		gd.free_inodes_count = (((sb->free_inode_count) + 10) / NUMBER_OF_GROUPS)*2; // check
		gd.directories_count = 0; // check
    	memcpy(&gdb->group_descriptor[i], &gd, sizeof(EXT2_GROUP_DESCRIPTOR));
    }
	return EXT2_SUCCESS;
}

int init_super_block(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb, UINT32 group_number)
{
	BYTE block[MAX_BLOCK_SIZE]; // 1KB
	ZeroMemory(block, sizeof(block));
	memcpy(block, sb, sizeof(block));
	block_write(disk, SUPER_BLOCK_BASE+(group_number*BLOCKS_PER_GROUP), block);
	ZeroMemory(block, sizeof(block));
	block_read(disk, SUPER_BLOCK_BASE+(group_number*BLOCKS_PER_GROUP), block);

	return EXT2_SUCCESS;
}

int init_gdt(DISK_OPERATIONS* disk, EXT2_GROUP_DESCRIPTOR_TABLE* gdt, UINT32 group_number)
{
	EXT2_GROUP_DESCRIPTOR gd[NUMBER_OF_GROUPS]; // 1 GDT
	BYTE block[MAX_BLOCK_SIZE];
	int gdt_block_num;
	EXT2_GROUP_DESCRIPTOR_TABLE* gdt_read= gdt;
	for(gdt_block_num=0; gdt_block_num < BLOCK_NUM_GDT_PER_GROUP; gdt_block_num++){
		ZeroMemory(block, sizeof(block));	
		memcpy(block, gdt_read+(gdt_block_num*MAX_BLOCK_SIZE), sizeof(block));
		block_write(disk, GDT_BASE+(group_number*BLOCKS_PER_GROUP)+gdt_block_num, block);
	}
	return EXT2_SUCCESS;
}

int init_block_bitmap(DISK_OPERATIONS* disk, UINT32 group_number)
{
	BYTE block[MAX_BLOCK_SIZE]; // 1 block bitmap
	ZeroMemory(block, sizeof(block));
	int i;
	for(i=0;i<DATA_BLOCK_BASE;i++)
		set_bit(i,block);
	block_write(disk, BLOCK_BITMAP_BASE+(group_number*BLOCKS_PER_GROUP), block);
	return EXT2_SUCCESS; 
}

int init_inode_bitmap(DISK_OPERATIONS* disk, UINT32 group_number)
{
	BYTE block[MAX_BLOCK_SIZE]; // 1 inode bitmap
	ZeroMemory(block, sizeof(block));
	int i;
	for(i = 0; i < 10 ; i++)
		set_bit(i, block);
	block_write(disk, INODE_BITMAP_BASE+(group_number*BLOCKS_PER_GROUP), block);
	return EXT2_SUCCESS;
}

int init_data_block(DISK_OPERATIONS* disk, UINT32 group_number)
{
	BYTE block[MAX_BLOCK_SIZE]; // 1 data block
	ZeroMemory(block, sizeof(block));
	block_write(disk, DATA_BLOCK_BASE+(group_number*BLOCKS_PER_GROUP), block);
	return EXT2_SUCCESS;
}

int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb)
{
	BYTE block[MAX_BLOCK_SIZE];
	SECTOR   rootBlock = 0;
	EXT2_DIR_ENTRY * entry;
	EXT2_GROUP_DESCRIPTOR * gd;
	EXT2_SUPER_BLOCK * sb_read;
	QWORD block_num_per_group = BLOCKS_PER_GROUP;
	INODE ip;
	int gi;

	ZeroMemory(block, MAX_BLOCK_SIZE);
	entry = (EXT2_DIR_ENTRY*)block;
	
	// entry 설정
	memset(entry->name,0x20,MAX_ENTRY_NAME_LENGTH);
	entry->name[0]='.';
	entry->name[1]='.';
	entry->inode = NUM_OF_ROOT_INODE;
	entry->name_len = 2;

	entry++;
	memset(entry->name,0x20,11);
	entry->name[0]='.';
	entry->inode = NUM_OF_ROOT_INODE;
	entry->name_len = 1;

	entry++;
	entry->name[0] = DIR_ENTRY_NO_MORE;

	rootBlock = 1 + sb->first_data_block_each_group;
	
	block_write(disk, rootBlock, block); // 루트 블럭에 정보 기록
	
	sb_read = (EXT2_SUPER_BLOCK *)block; // 슈퍼블록으로 참조가능
	for (gi = 0; gi < NUMBER_OF_GROUPS; gi++) // 모든 그룹에 적용
	{
		block_read(disk, block_num_per_group * gi + SUPER_BLOCK_BASE, block); 	// 각 블록그룹의 슈퍼블록을 읽어옴
		sb_read->free_block_count--;											// 프리 블럭 줄여줌
		block_write(disk, block_num_per_group * gi + SUPER_BLOCK_BASE, block);   // 다시 써줌
	}

	// 그룹 디스크립터에 수정 과정
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(disk, GDT_BASE, block);

	gd->free_blocks_count--;
	gd->directories_count = 1; // root dir

	for (gi = 0; gi < NUMBER_OF_GROUPS; gi++) // 각 그룹 디스크립터 count-- 한 것으로 바꿔 줌
		block_write(disk, block_num_per_group * gi + GDT_BASE, block);

	block_read(disk, BLOCK_BITMAP_BASE, block);
	set_bit(rootBlock,block);
	//block[2] |= 0x02;  // sb->first_data_block_each_group
	block_write(disk, BLOCK_BITMAP_BASE, block);

	ZeroMemory(block, MAX_BLOCK_SIZE);
	ip.mode = 0x1FF | 0x4000; // 권한 부여와 디렉토리로 모드 비트를 설정해준다.
	ip.size = 0; 
	ip.blocks = 1; // 파일 데이터를 저장하는데 필요한 개수
	ip.block[0] = DATA_BLOCK_BASE; 
	memcpy(block+sizeof(INODE),&ip,sizeof(INODE));
	block_write(disk, INODE_TABLE_BASE, block);
	
	return EXT2_SUCCESS;
}


/******************************************************************************/
/*																			  */
/*								read function 								  */
/*																			  */
/******************************************************************************/
int read_superblock(EXT2_FILESYSTEM* fs, EXT2_NODE* root)
{
	INT result;
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_DIR_ENTRY entry;
    EXT2_DIR_ENTRY_LOCATION location;
	char name[MAX_NAME_LENGTH];

	if (fs == NULL || fs->disk == NULL)
	{
		WARNING("DISK OPERATIONS : %p\nEXT2_FILESYSTEM : %p\n", fs, fs->disk);
		return EXT2_ERROR;
	}
	// 슈퍼블럭의 data를 읽어서 EXT2_FILESYSTEM을 넣어준다
	if (block_read(fs->disk, SUPER_BLOCK_BASE, block))//if (block_read(fs->disk, SUPER_BLOCK_BASE, &fs->sb)) //오류..
        return EXT2_ERROR;
	
    if(validate_sb(block))
		return EXT2_ERROR;

	memcpy(&fs->sb, block , sizeof(block));//memcpy(block, &fs->sb, sizeof(block));//???
	
	// gd를 읽어서 EXT2_FILESYSTEM을 넣어준다
	ZeroMemory(block, sizeof(block));
    block_read(fs->disk, GDT_BASE, block);
    memcpy(&fs->gd, block, sizeof(EXT2_GROUP_DESCRIPTOR));
	
	// root를 읽어서 EXT2_FILESYSTEM을 넣어준다 
	ZeroMemory(block, sizeof(MAX_BLOCK_SIZE));
	if (read_root_block(fs, block))
		return EXT2_ERROR;
	ZeroMemory(root, sizeof(EXT2_NODE));
	memcpy(&root->entry, block, sizeof(EXT2_DIR_ENTRY));

	memset(name,SPACE_CHAR,sizeof(name));
	entry.inode = 2;
    entry.name_len = 1;
	entry.name[0] = '/';
    location.group = 0;
    location.block = DATA_BLOCK_BASE;
    location.offset = 0; 
    
    root->fs = fs;
    root->entry = entry;
    root->location = location;

	return EXT2_SUCCESS;
}

int read_root_block(EXT2_FILESYSTEM* fs, BYTE* block)
{
	UINT32 inode = 2;
	INODE inodeBuffer;
	SECTOR rootBlock;
	get_inode(fs, inode, &inodeBuffer);
	rootBlock = inodeBuffer.block[0];
	return block_read(fs->disk, rootBlock, block);
}

int read_dir_from_block(EXT2_FILESYSTEM* fs, BYTE* block, EXT2_NODE_ADD adder, void* list)
{
	EXT2_DIR_ENTRY* dir_entry;
	EXT2_NODE node;
	dir_entry = (EXT2_DIR_ENTRY*)block;
	UINT i=0;

	while (dir_entry[i].name[0] != DIR_ENTRY_NO_MORE) // not end of entry in block
    {
        if ( (dir_entry[i].name[0] != '.') && (dir_entry[i].name[0] != DIR_ENTRY_FREE))
        {
            node.entry = dir_entry[i];
            node.fs = fs;
            node.location.offset = i;
            adder(list, &node);
        }
        i++;
    }
	return EXT2_SUCCESS;
}


/******************************************************************************/
/*																			  */
/*								get function 								  */
/*																			  */
/******************************************************************************/
int get_inode(EXT2_FILESYSTEM* fs, UINT32 inode_num, INODE *inodeBuffer)
{
    BYTE block[MAX_BLOCK_SIZE];
	inode_num--;
	UINT32 group_number = inode_num / INODES_PER_GROUP;  // 그룹 번호
	UINT32 group_inode_offset = inode_num % INODES_PER_GROUP ; // 그룹 내 아이노드의 오프셋 
	UINT32 block_number = group_inode_offset / INODES_PER_BLOCK; // inode_table안에서 블럭 번호
	UINT32 block_inode_offset = group_inode_offset % INODES_PER_BLOCK; // 블럭 안에서 아이노드의 오프셋
	
	block_read(fs->disk, INODE_TABLE_BASE + (group_number * BLOCKS_PER_GROUP) + block_number, block); // 해당 INODE를 읽어오기
	memcpy(inodeBuffer, block+(INODE_SIZE*(block_inode_offset)) , sizeof(INODE)); 
}

int lookup_entry(EXT2_FILESYSTEM* fs, int inode, char* name, EXT2_NODE* retEntry) 
{
	
	SECTOR block[MAX_BLOCK_SIZE]; // block buffer
	INODE inodeBuffer;
	EXT2_DIR_ENTRY* entry;
	INT32 lastEntry = MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY) - 1; // 1024/32 - 1 = 32 - 1 
	UINT32 i,result,number;
	UINT32 begin =0;

	get_inode(fs, inode, &inodeBuffer);
	
	for (i = 0; i < inodeBuffer.blocks; i++)
	{
		block_read(fs->disk, inodeBuffer.block[i], block); // inode번호
		entry = (EXT2_DIR_ENTRY*)block;
		retEntry->location.block = inodeBuffer.block[i]; // 몇번째 블록이냐, (블록 넘버가 아니고, 이 파일이 쓰고있는 블록중)
		result = find_entry_at_block(block, name, begin, lastEntry, &number);
		
		switch (result)
		{
		case -2: { return EXT2_ERROR; } // NO MORE인 것을  만났을 때
		case -1: { continue; } // 계속 탐색
		case  0: { // 동일한 이름 존재
				memcpy( &retEntry->entry, &entry[number], sizeof(EXT2_DIR_ENTRY)); // 찾은 엔트리 위치의 구조체 내용을 현재 엔트리에 넣어준다.
	 			retEntry->location.group = GET_INODE_GROUP(entry[number].inode);
	 			retEntry->location.block = inodeBuffer.block[i]; 
	 			retEntry->location.offset = number;
	 			retEntry->fs = fs;
 				return EXT2_SUCCESS; 
		 	}
		}
	}
	return EXT2_ERROR; // entry가 꽉 찼다.
}

int find_entry_at_block(BYTE* block, char* formattedName, UINT32 begin, UINT32 last, UINT32* number)
{
	UINT32	i;
	EXT2_DIR_ENTRY* entry = ( EXT2_DIR_ENTRY* )block;

	for( i = begin; i < last; i++){
		if( formattedName == NULL ) // 디렉토리에 무언가라도 존재하는지 검사
		{
			if(entry->name[0] != DIR_ENTRY_NO_MORE && entry->name[0] != DIR_ENTRY_FREE && (entry->name[0] != '.') )
			{
				*number = i;
				return EXT2_SUCCESS;
			}
		}
		else
		{	
			if( memcmp(entry->name, formattedName, MAX_ENTRY_NAME_LENGTH) == 0)	
			{
				*number = i;
				return EXT2_SUCCESS; // 동일한 이름을 찾았을 때 성공 리턴
			}
			if((formattedName[0] == DIR_ENTRY_FREE || formattedName[0]==DIR_ENTRY_NO_MORE ) && ( formattedName[0] == entry->name[0]) )
			{   
				*number = i;
				return EXT2_SUCCESS; // 프리나 노모어의 위치를 찾을 때 성공 리턴
			}
		}
		if( entry->name[0] == DIR_ENTRY_NO_MORE ) // 해당 블럭 디렉토리의 끝까지 검사 했을 때 
		{
			*number = i;
			return -2; // 동일한 이름을 못 찾았다는 의미
		}
		entry++;
	}
	*number = i;
	 return EXT2_ERROR; // 블럭이 꽉찬 것을 의미
}

// 비어 있는 inode를 리턴 받으면서 해당 inode bitmap을 setting
UINT32 get_free_inode_number(EXT2_FILESYSTEM* fs)
{
    EXT2_INODE_BITMAP i_bitmap;
    EXT2_BITSET inodeset;
    int i,j;
	unsigned char k=0x80; // 1000 0000

	block_read(fs->disk, (fs->sb.block_group_number*BLOCKS_PER_GROUP) + INODE_BITMAP_BASE, &i_bitmap);
    
    for (i = 0; i < fs->sb.inode_per_group && i < MAX_BLOCK_SIZE/4; i++)
    {
        if (i_bitmap.inode_bitmap[i] != 0xff)
        {
            for (j = 0; j < 8; j++) // 8bit means 1byte
            {
                if ( !(i_bitmap.inode_bitmap[i] & (k >> j)) )
                {
					// 선형 비트 탐색 알고리즘
                    inodeset.bit_num = 8 * i + j; // this is free inode bit!
                    inodeset.index_num = i; // bitmap index num
                    set_inode_bitmap(fs, &i_bitmap, inodeset);
                    return inodeset.bit_num+1; // inode는 0번 부터가 아닌 1번 부터 시작이므로 free number에 1을 더해준다.
                }
            }
        }
    }
    return 0;
}

// 비어 있는 data block을 리턴 받으면서 해당 block bitmap을 setting
UINT32 get_free_block_number(EXT2_FILESYSTEM* fs)
{
    EXT2_BLOCK_BITMAP b_bitmap;
    EXT2_BITSET blockset;
	int i,j;
	unsigned char k=0x80;

    block_read(fs->disk, (fs->sb.block_group_number*BLOCKS_PER_GROUP)+BLOCK_BITMAP_BASE ,&b_bitmap);
    
    //find empty bit in block bitmap
    for (i = 0; i < fs->sb.block_per_group && i < MAX_BLOCK_SIZE/4; i++)
    {
    if (b_bitmap.block_bitmap[i] != 0xff)
    	{
        for (j = 0; j < 8; j++)
        {
            if ( !(b_bitmap.block_bitmap[i] & (k >> j)) )
            {
					// 선형 비트 탐색 알고리즘
                    blockset.bit_num = 8 * i + j;
                    blockset.index_num = i;
                    set_block_bitmap(fs, &b_bitmap, blockset);
                    return blockset.bit_num;
            }
        }
    	}
    }
    return 0;
}

int get_data_block_at_inode(EXT2_FILESYSTEM *fs, INODE inode, UINT32 number) 
{
	return inode.block[number];
}


/******************************************************************************/
/*																			  */
/*								set function 								  */
/*																			  */
/******************************************************************************/
int insert_entry(EXT2_NODE * parent, EXT2_NODE * newEntry, UINT16 fileType)
{

	EXT2_DIR_ENTRY_LOCATION begin;
	EXT2_NODE entry;
	char entryName[2]={ 0, };
	INODE inodeBuffer;
	INT32 free_block_num,free_inode_num;
	entryName[0] = DIR_ENTRY_FREE;
	if(lookup_entry(parent->fs, parent->entry.inode, entryName, &entry )==EXT2_SUCCESS)
	{
		
		set_entry(parent->fs, &entry.location, &newEntry->entry );
		newEntry->location = entry.location;
	}
	else{
		entryName[0] = DIR_ENTRY_NO_MORE;
		if(lookup_entry(parent->fs, parent->entry.inode, entryName, &entry )==EXT2_ERROR)
			return EXT2_ERROR; // no_more 못 찾았으면 오류
	
		set_entry(parent->fs, &entry.location, &newEntry->entry );
		newEntry->location = entry.location;
		entry.location.offset++; // no_more위치 +1에 다시 표시해줘야함 no_more를

		if(entry.location.offset == MAX_BLOCK_SIZE/sizeof(EXT2_DIR_ENTRY)) // 블럭당 엔트리 꽉차면
		{
			get_inode(parent->fs, parent->entry.inode, &inodeBuffer); // 부모 아이노드의 받아와서
			free_block_num = get_free_block_number(parent->fs); // free블럭 받아오고
			
			inodeBuffer.block[inodeBuffer.blocks-1] = free_block_num; // 간접 참조를 생각하지 않은 할당
			//indirect 영역
			entry.location.block = free_block_num;
			entry.location.offset = 0;
			
		}
		set_entry(parent->fs,&entry.location, &entry.entry );
	}
	return EXT2_SUCCESS;
}

int insert_inode_table(EXT2_NODE* parent, INODE* inode_entry, int free_inode)
{
	DISK_OPERATIONS* disk;
    disk = parent->fs->disk;
	BYTE block[MAX_BLOCK_SIZE];
	INODE* inode;
	free_inode--;

	UINT32 group_num = free_inode / INODES_PER_GROUP;
    UINT32 block_num = (free_inode % INODES_PER_GROUP) / INODES_PER_BLOCK;
	UINT32 block_offset = (free_inode % INODES_PER_GROUP) % INODES_PER_BLOCK;

	block_read(disk, (group_num * BLOCKS_PER_GROUP) + INODE_TABLE_BASE + block_num, block);
	
	inode = (INODE*)block;
	inode = inode + block_offset;
	*inode = *inode_entry;
   
    block_write(disk, (group_num * BLOCKS_PER_GROUP) + INODE_TABLE_BASE + block_num, block);
    
    return EXT2_SUCCESS;
}

int set_sb_free_block_count(EXT2_NODE* node, int num)
{
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_SUPER_BLOCK* sb;
	DISK_OPERATIONS* disk = node->fs->disk;
	block_read(disk,SUPER_BLOCK_BASE,block);
	sb = (EXT2_SUPER_BLOCK*)block;
	
	if( num > 0 )
		sb->free_block_count += num;
	else if( sb->free_block_count >= -num){
		sb->free_block_count += num;
	}
	else{
		PRINTF("No more free_block exist\n");
		return EXT2_ERROR;
	}
	block_write(disk,SUPER_BLOCK_BASE,block);
}

int set_sb_free_inode_count(EXT2_NODE* node ,int num)
{
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_SUPER_BLOCK* sb;
	DISK_OPERATIONS* disk = node->fs->disk;

	block_read(disk,SUPER_BLOCK_BASE,block);
	sb = (EXT2_SUPER_BLOCK*)block;
	if( num > 0 )
		sb->free_inode_count += num;
	else if( sb->free_inode_count >= -num){
		sb->free_inode_count += num;
	}
	else{
		PRINTF("No more free_inode exist\n");
		return EXT2_ERROR;
	}
	block_write(disk,SUPER_BLOCK_BASE,block);
}

int set_gd_free_block_count(EXT2_NODE* node,UINT32 free_data_block,int num)
{
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_GROUP_DESCRIPTOR* gd;
	DISK_OPERATIONS* disk = node->fs->disk;
	UINT32 group_num = GET_DATA_GROUP(free_data_block);
	UINT32 gd_block_num = group_num*sizeof(EXT2_GROUP_DESCRIPTOR) / MAX_BLOCK_SIZE;
	UINT32 gd_block_offset = group_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR));
	//그룹디스크립터의 블럭 번호를 구해준다

	block_read(disk,GDT_BASE+gd_block_num,block);
	gd = (EXT2_GROUP_DESCRIPTOR*)block;
	gd += gd_block_offset;
	if( num > 0 )
		gd->free_blocks_count += num;
	else if( gd->free_blocks_count >= -num){
		gd->free_blocks_count += num;
	}
	else{
		PRINTF("No more free_block exist\n");
		return EXT2_ERROR;
	}
	block_write(disk,GDT_BASE+gd_block_num,block);
}

int set_gd_free_inode_count(EXT2_NODE* node,UINT32 free_inode,int num)
{
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_GROUP_DESCRIPTOR* gd;
	DISK_OPERATIONS* disk = node->fs->disk;
	UINT32 group_num = GET_INODE_GROUP(free_inode); // 그룹 번호 구해줌
	UINT32 gd_block_num = group_num*sizeof(EXT2_GROUP_DESCRIPTOR) / MAX_BLOCK_SIZE; // 블럭 번호
	UINT32 gd_block_offset = group_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)); // 블럭 내에서 위치를 구해준다

	block_read(disk, GDT_BASE + gd_block_num, block);//해당 블럭읽음
	gd = (EXT2_GROUP_DESCRIPTOR*)block;
	gd += gd_block_offset; //해당 오프셋으로 점프

	if( num > 0 )
		gd->free_inodes_count += num; // free_inode 증가시
	else if( gd->free_blocks_count >= -num)
		gd->free_inodes_count += num; // free_inode 감소시
	else{
		PRINTF("No more free_block exist\n");
		return EXT2_ERROR;
	}
	block_write(disk, GDT_BASE + gd_block_num, block);
}

// 해당 location에 위치한 entry를 block에 setting
int set_entry( EXT2_FILESYSTEM* fs,EXT2_DIR_ENTRY_LOCATION* location, EXT2_DIR_ENTRY* value )
{
	BYTE block[MAX_BLOCK_SIZE];
	EXT2_DIR_ENTRY* entry;
	int result;
	result = block_read(fs->disk, location->block,block);
	entry = ( EXT2_DIR_ENTRY* )block;
	entry[location->offset] = *value;
	result = block_write(fs->disk,location->block,block);//SUPER_BLOCK_BASE+(location->group*BLOCKS_PER_GROUP)+

	return result;
}

int set_inode_bitmap(EXT2_FILESYSTEM* fs, EXT2_INODE_BITMAP* i_bitmap, EXT2_BITSET bitset)
{
    DISK_OPERATIONS* disk = fs->disk;
    i_bitmap->inode_bitmap[bitset.index_num] ^= (0x80 >> ((bitset.bit_num % 8)));
    block_write(disk, (fs->sb.block_group_number*BLOCKS_PER_GROUP)+INODE_BITMAP_BASE, i_bitmap);
}

int set_block_bitmap(EXT2_FILESYSTEM* fs, EXT2_BLOCK_BITMAP* b_bitmap, EXT2_BITSET bitset)
{
    DISK_OPERATIONS* disk = fs->disk;
    b_bitmap->block_bitmap[bitset.index_num] ^= (0x80 >> ((bitset.bit_num % 8)));
    block_write(disk, (fs->sb.block_group_number*BLOCKS_PER_GROUP)+BLOCK_BITMAP_BASE, b_bitmap);
}

void fill_data_block(DISK_OPERATIONS* disk,INODE*inodetemp, unsigned long length, unsigned long offset, const char* buffer, UINT32 block_num)
{
	BYTE block[MAX_BLOCK_SIZE];
    UINT32 copy_num= MAX_BLOCK_SIZE / SHELL_BUFFER_SIZE;
    int i,j;
    unsigned long lesslength = length;
    unsigned long cur_offset = offset;

	
    if (block_num != 0) // 블락 단위 write
	{
    	for(i=0; i<block_num+1; i++)
    	{
        	ZeroMemory(block, sizeof(block));
        	cur_offset = 0;
        	for(j=0; j<copy_num; j++)
        	{
            	memcpy(&block[cur_offset], buffer, SHELL_BUFFER_SIZE);
            	cur_offset += SHELL_BUFFER_SIZE;
        	}
        	block_write(disk, inodetemp->block[i], block);
        	lesslength -= MAX_BLOCK_SIZE;
    	}
	}
	else{ // 1 블락 보다 작은 것들에 대한 write (잉여물)
    	ZeroMemory(block, sizeof(block));
		cur_offset = 0;
    	for(i=0; i< ((lesslength + SHELL_BUFFER_SIZE -1) / SHELL_BUFFER_SIZE); i++)
    	{
        	memcpy(&block[cur_offset], buffer, SHELL_BUFFER_SIZE);
        	cur_offset += SHELL_BUFFER_SIZE;
    	}
    	block_write(disk,inodetemp->block[block_num],block);
	}
}


/******************************************************************************/
/*																			  */
/*							utility function 								  */
/*																			  */
/******************************************************************************/
int format_name(EXT2_FILESYSTEM* fs, char* name)
{
	UINT32	i, length;
	UINT32	extender = 0, nameLength = 0;
	UINT32	extenderCurrent = 8;
	char	regularName[MAX_ENTRY_NAME_LENGTH];

	memset(regularName, 0x20, sizeof(regularName));
	length = strlen(name);

	if (strncmp(name, "..", 2) == 0)
	{
		memcpy(name, "..         ", 11);
		return EXT2_SUCCESS;
	}
	else if (strncmp(name, ".", 1) == 0)
	{
		memcpy(name, ".          ", 11);
		return EXT2_SUCCESS;
	}
	else
	{
		upper_string(name, MAX_ENTRY_NAME_LENGTH);
		for (i = 0; i < length; i++)
		{
			if (name[i] != '.' && !isdigit(name[i]) && !isalpha(name[i]))
				return EXT2_ERROR;

			if (name[i] == '.')
			{
				if (extender)
					return EXT2_ERROR;
				extender = 1;
			}
			else if (isdigit(name[i]) || isalpha(name[i]))
			{
				if (extender)
					regularName[extenderCurrent++] = name[i];
				else
					regularName[nameLength++] = name[i];
			}
			else
				return EXT2_ERROR;
		}

		if (nameLength > 8 || nameLength == 0 || extenderCurrent > 11)
			return EXT2_ERROR;
	}

	memcpy(name, regularName, sizeof(regularName));
	return EXT2_SUCCESS;
}

int block_write(DISK_OPERATIONS* this, SECTOR block, void* data)
{
	int i;
	int result;
	int sectorNum = block * SECTORS_PER_BLOCK;

	for (i = 0; i<SECTORS_PER_BLOCK ; i++){
		result=disksim_write(this, sectorNum+i, data+(i*MAX_SECTOR_SIZE));
	}
	return result;
}

int block_read(DISK_OPERATIONS* this, SECTOR block, void* data)
{
	int i;
	int result; 
	int sectorNum = block*SECTORS_PER_BLOCK;

	for(i = 0; i<SECTORS_PER_BLOCK ; i++){
		result=disksim_read(this, sectorNum+i, data+(i*MAX_SECTOR_SIZE));
	}
	return result;
}

int set_bit(SECTOR number, BYTE* bitmap)
{ 
	BYTE value;
	SECTOR byte = number / 8;
	SECTOR offset = number % 8;

	switch(offset){
		case 0:
			value = 0x80; // 1000 0000
			break;
		case 1:
			value = 0x40; // 0100 0000
			break;
		case 2:
			value = 0x20; // 0010 0000
			break;
		case 3:
			value = 0x10; // 0001 0000
			break;
		case 4:
			value = 0x8; // 0000 1000
			break;
		case 5:
			value = 0x4; // 0000 0100
			break;
		case 6:
			value = 0x2; // 0000 0010
			break;
		case 7:
			value = 0x1; // 0000 0001
			break;
	}
	bitmap[byte] |= value;
}

int set_zero_bit(SECTOR number, BYTE* bitmap)
{ 
	BYTE value;
	SECTOR byte = number / 8;
	SECTOR offset = number % 8;

	switch(offset){
		case 0:
			value = 0x80; // 1000 0000
			break;
		case 1:
			value = 0x40; // 0100 0000
			break;
		case 2:
			value = 0x20; // 0010 0000
			break;
		case 3:
			value = 0x10; // 0001 0000
			break;
		case 4:
			value = 0x8; // 0000 1000
			break;
		case 5:
			value = 0x4; // 0000 0100
			break;
		case 6:
			value = 0x2; // 0000 0010
			break;
		case 7:
			value = 0x1; // 0000 0001
			break;
	}
	bitmap[byte] ^= value;
}

int dump_memory(DISK_OPERATIONS* disk, int block) 
{
    
	BYTE dump[1024]; // 1 block
	ZeroMemory(dump,sizeof(dump));
    block_read(disk, block, dump);
    int i;
	for (i = 0; i<64; i++)
    {
		int j;
		printf("%p\t ",&dump[i*16]);
        for (j = (16 * i); j<(16 * i) + 16; j++)
        {
            printf("%02x  ", dump[j]);
        }
        printf("\n");
    }
    return EXT2_SUCCESS;
}

int my_strnlen(char* src)   
{
	int num=0;
	while (*src && *src != 0x20){
		*src++;
		num++;
	}
	return num;
}

void upper_string(char* str, int length)
{
	while (*str && length-- > 0)
	{
		*str = toupper(*str);
		str++;
	}
}

int validate_sb(void* block) 
{
    EXT2_SUPER_BLOCK* sb = (EXT2_SUPER_BLOCK*)block;
    
    if (!(sb->magic_signature == 0xEF53)) // 간단하게 magic_signature만 검사하고 종료 우리 시스템 상에서 많은 비교가 불필요
        return EXT2_ERROR;
    
    return EXT2_SUCCESS;
}