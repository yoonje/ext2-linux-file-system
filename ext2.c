typedef struct {
    char *address;
} DISK_MEMORY;

#include "ext2.h"
#include "disksim.h"

#define MIN(a, b)                    ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX(a, b)                    ( ( a ) > ( b ) ? ( a ) : ( b ) )


/******************************************************************************/
/*																			  */
/*								EXT2_COMMAND 								  */
/*																			  */
/******************************************************************************/
int ext2_write(EXT2_NODE *file, unsigned long offset, unsigned long length, char *buffer) {
    BYTE block[MAX_BLOCK_SIZE];
    INODE nodeBuffer;
    int i;
    EXT2_DIR_ENTRY *dir;
    EXT2_DIR_ENTRY my_entry;
    INT32 block_num;
    DISK_OPERATIONS *disk = file->fs->disk;
    INODE curr_inode;

    block_num = length / MAX_BLOCK_SIZE;
    block_read(disk, file->location.block, block);
    dir = (EXT2_DIR_ENTRY *) block;
    my_entry = dir[file->location.offset];

    get_inode(file->fs, my_entry.inode, &curr_inode);
    curr_inode.size = length;

    if (block_num < EXT2_D_BLOCKS) {
        curr_inode.blocks = block_num;
        for (i = 0; i < block_num - 1; i++) {
            curr_inode.block[i + 1] = get_free_block_number(file->fs);
        }
    } else {
        return EXT2_ERROR;
    }

    fill_data_block(disk, &curr_inode, length, offset, buffer, block_num);
    insert_inode_table(file, &curr_inode, my_entry.inode);

    return EXT2_SUCCESS;
}


int ext2_read(EXT2_NODE *EXT2Entry, int offset, unsigned long length, char *buffer) {
    BYTE block[MAX_BLOCK_SIZE];
    int count = offset / MAX_BLOCK_SIZE;
    INODE inodeBuffer;

    get_inode(EXT2Entry->fs, EXT2Entry->entry.inode, &inodeBuffer);
    block_read(EXT2Entry->fs, inodeBuffer.block[count], block);
    memcpy(buffer, block, MAX_BLOCK_SIZE - 1);

    if (inodeBuffer.blocks + 1 > count)
        return 1;
    return -1;
}

int ext2_rmdir(EXT2_NODE *parent, EXT2_NODE *rmdir) {
    DISK_OPERATIONS *disk = parent->fs->disk;
    INODE inodeBuffer;
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_DIR_ENTRY *dir;
    INT32 count = 1;
    INT32 inodenum, groupnum;
    char *entryName = NULL;
    EXT2_NODE bufferNode;

    rmdir->fs = parent->fs;
    int i, bitmap_num;

    get_inode(parent->fs, rmdir->entry.inode, &inodeBuffer);
    if (!(inodeBuffer.mode & FILE_TYPE_DIR)) {
        printf("this is not type DIR!!\n");
        return EXT2_ERROR;
    }

    if (lookup_entry(parent->fs, rmdir->entry.inode, entryName, &bufferNode) == EXT2_SUCCESS)
        return EXT2_ERROR;

    for (i = 0; i < inodeBuffer.blocks; i++) {
        ZeroMemory(block, MAX_BLOCK_SIZE);
        if (i <= EXT2_D_BLOCKS) {
            block_write(disk, inodeBuffer.block[i], block);
            set_gd_free_block_count(rmdir, inodeBuffer.block[i], count);
            set_sb_free_block_count(rmdir, count);

            block_read(disk, BLOCKS_PER_GROUP * GET_DATA_GROUP(inodeBuffer.block[i]) + BLOCK_BITMAP_BASE, block);
            bitmap_num = (inodeBuffer.block[i] % (BLOCKS_PER_GROUP)) - DATA_BLOCK_BASE;
            set_zero_bit(bitmap_num, block);//----------------------------
            block_write(disk, BLOCKS_PER_GROUP * GET_DATA_GROUP(inodeBuffer.block[i]) + BLOCK_BITMAP_BASE, block);
        } else {
        }
    }

    block_read(disk, rmdir->location.block, block);
    dir = (EXT2_DIR_ENTRY *) block;
    dir += rmdir->location.offset;

    inodenum = dir->inode;
    groupnum = inodenum / INODES_PER_GROUP;

    ZeroMemory(dir, sizeof(EXT2_DIR_ENTRY));
    block_write(disk, rmdir->location.block, block);
    dir->name[0] = DIR_ENTRY_FREE;
    set_entry(parent->fs, &rmdir->location, dir);


    ZeroMemory(&inodeBuffer, sizeof(INODE));
    insert_inode_table(parent, &inodeBuffer, inodenum);

    block_read(disk, groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);
    bitmap_num = (inodenum % INODES_PER_GROUP) - 1;
    set_zero_bit(bitmap_num, block);
    block_write(disk, groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);


    set_sb_free_inode_count(rmdir, count);
    set_gd_free_inode_count(rmdir, inodenum, count);

    return EXT2_SUCCESS;
}

int ext2_remove(EXT2_NODE *parent, EXT2_NODE *rmfile) {
    DISK_OPERATIONS *disk = parent->fs->disk;
    INODE inodeBuffer;
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_DIR_ENTRY *dir;
    INT32 count = 1;
    INT32 inodenum, groupnum;

    rmfile->fs = parent->fs;
    INT32 i, bitmap_num;

    get_inode(parent->fs, rmfile->entry.inode, &inodeBuffer);

    if (inodeBuffer.mode & FILE_TYPE_DIR) {
        printf("type DIR!!\n");
        return EXT2_ERROR;
    }


    for (i = 0; i < inodeBuffer.blocks; i++) {
        ZeroMemory(block, MAX_BLOCK_SIZE);
        if (i <= EXT2_D_BLOCKS) {
            block_write(disk, inodeBuffer.block[i], block);
            set_gd_free_block_count(rmfile, inodeBuffer.block[i], count);
            set_sb_free_block_count(rmfile, count);

            block_read(disk, BLOCKS_PER_GROUP * GET_DATA_GROUP(inodeBuffer.block[i]) + BLOCK_BITMAP_BASE, block);
            bitmap_num = (inodeBuffer.block[i] % (BLOCKS_PER_GROUP)) - DATA_BLOCK_BASE;
            set_zero_bit(bitmap_num, block);
            block_write(disk, BLOCKS_PER_GROUP * GET_DATA_GROUP(inodeBuffer.block[i]) + BLOCK_BITMAP_BASE, block);
        } else {
        }
    }

    block_read(disk, rmfile->location.block, block);
    dir = (EXT2_DIR_ENTRY *) block;
    dir += rmfile->location.offset;

    inodenum = dir->inode;
    groupnum = inodenum / INODES_PER_GROUP;

    ZeroMemory(dir, sizeof(EXT2_DIR_ENTRY));
    block_write(disk, rmfile->location.block, block);
    dir->name[0] = DIR_ENTRY_FREE;
    set_entry(parent->fs, &rmfile->location, dir);


    ZeroMemory(&inodeBuffer, sizeof(INODE));
    insert_inode_table(parent, &inodeBuffer, inodenum);

    block_read(disk, groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);
    bitmap_num = (inodenum % INODES_PER_GROUP) - 1;
    set_zero_bit(bitmap_num, block);
    block_write(disk, groupnum * BLOCKS_PER_GROUP + INODE_BITMAP_BASE, block);


    set_sb_free_inode_count(rmfile, count);
    set_gd_free_inode_count(rmfile, inodenum, count);

    return EXT2_SUCCESS;
}

int ext2_mkdir(EXT2_NODE *parent, char *entryName, EXT2_NODE *retEntry) {
    EXT2_NODE dotNode, dotdotNode;
    DWORD firstCluster;
    char name[MAX_ENTRY_NAME_LENGTH];
    int result;
    int i;
    int free_inode, free_data_block, count;
    INODE inode_entry;

    strcpy((char *) name, entryName);

    format_name(parent->fs, (char *) name);

    free_inode = get_free_inode_number(parent->fs);
    free_data_block = get_free_block_number(parent->fs);

    count = -1;
    retEntry->fs = parent->fs;
    set_sb_free_block_count(retEntry, count);
    set_sb_free_inode_count(retEntry, count);

    set_gd_free_block_count(retEntry, free_data_block, count);
    set_gd_free_inode_count(retEntry, free_inode, count);

    ZeroMemory(retEntry, sizeof(EXT2_NODE));
    memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH);
    retEntry->entry.name_len = strlen((char *) retEntry->entry.name);
    retEntry->entry.inode = free_inode;
    retEntry->fs = parent->fs;
    result = insert_entry(parent, retEntry, FILE_TYPE_DIR);

    if (result == EXT2_ERROR)
        return EXT2_ERROR;

    ZeroMemory(&inode_entry, sizeof(INODE));
    inode_entry.block[0] = free_data_block;
    inode_entry.mode = USER_PERMITION | FILE_TYPE_DIR;
    inode_entry.blocks = 1;
    inode_entry.size = 0;

    insert_inode_table(parent, &inode_entry, free_inode);

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

int ext2_read_dir(EXT2_NODE *dir, EXT2_NODE_ADD adder, void *list) {
    BYTE block[MAX_BLOCK_SIZE];
    INODE *inodeBuffer;
    int i, result, block_num;
    UINT32 group_num;

    group_num = dir->fs->sb.block_group_number;
    inodeBuffer = (INODE *) malloc(sizeof(INODE));

    ZeroMemory(block, MAX_BLOCK_SIZE);
    ZeroMemory(inodeBuffer, sizeof(INODE));
    result = get_inode(dir->fs, dir->entry.inode, inodeBuffer);

    if (result == EXT2_ERROR)
        return EXT2_ERROR;

    for (i = 0; i < inodeBuffer->blocks; ++i) {
        block_num = get_data_block_at_inode(dir->fs, *inodeBuffer, i);
        block_read(dir->fs->disk, block_num, block);
        read_dir_from_block(dir->fs, block, adder, list);
    }
    return EXT2_SUCCESS;
}

int ext2_format(DISK_OPERATIONS *disk) {
    EXT2_SUPER_BLOCK sb;
    EXT2_GROUP_DESCRIPTOR_TABLE gdt;
    EXT2_INODE_BITMAP ib;
    EXT2_BLOCK_BITMAP bb;
    EXT2_INODE_TABLE it;
    int i;

    if (fill_super_block(&sb) != EXT2_SUCCESS)
        return EXT2_ERROR;
    if (fill_descriptor_table(&gdt, &sb) != EXT2_SUCCESS)
        return EXT2_ERROR;
    init_inode_bitmap(disk, i);

    for (i = 0; i < NUMBER_OF_GROUPS; i++) {
        sb.block_group_number = i;
        init_super_block(disk, &sb, i);
        init_gdt(disk, &gdt, i);
        init_block_bitmap(disk, i);
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

int ext2_lookup(EXT2_NODE *parent, char *entryName, EXT2_NODE *retEntry) {
    EXT2_DIR_ENTRY_LOCATION begin;
    char formattedName[MAX_NAME_LENGTH] = {0,};

    strncpy(formattedName, entryName, MAX_ENTRY_NAME_LENGTH);
    format_name(parent->fs, formattedName);

    int result = lookup_entry(parent->fs, parent->entry.inode, formattedName,
                              retEntry);

    return result;
}

int ext2_create(EXT2_NODE *parent, char *entryName, EXT2_NODE *retEntry) {
    EXT2_DIR_ENTRY dir_entry;
    UINT32 inode;
    char name[MAX_ENTRY_NAME_LENGTH] = {0,};
    BYTE block[MAX_BLOCK_SIZE];
    INODE inode_entry;
    UINT32 result;
    UINT32 free_inode;
    UINT32 free_data_block;
    INT32 count;

    if ((parent->fs->gd.free_inodes_count) == 0) return EXT2_ERROR;

    strncpy(name, entryName, MAX_ENTRY_NAME_LENGTH);

    if (format_name(parent->fs, name) == EXT2_ERROR) return EXT2_ERROR;

    ZeroMemory(retEntry, sizeof(EXT2_NODE));
    memcpy(retEntry->entry.name, entryName, MAX_ENTRY_NAME_LENGTH);
    if (ext2_lookup(parent, name, retEntry) == EXT2_SUCCESS) return EXT2_ERROR;

    retEntry->fs = parent->fs;

    free_inode = get_free_inode_number(parent->fs);
    free_data_block = get_free_block_number(parent->fs);

    count = -1;
    set_sb_free_block_count(retEntry, count);
    set_sb_free_inode_count(retEntry, count);

    set_gd_free_block_count(retEntry, free_data_block, count);
    set_gd_free_inode_count(retEntry, free_inode, count);

    ZeroMemory(&inode_entry, sizeof(INODE));
    inode_entry.block[0] = free_data_block;
    inode_entry.mode = USER_PERMITION | 0x2000;
    inode_entry.blocks = 1;
    inode_entry.size = 0;
    insert_inode_table(parent, &inode_entry, free_inode);

    ZeroMemory(&dir_entry, sizeof(EXT2_DIR_ENTRY));
    memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH);
    retEntry->entry.name_len = my_strnlen(retEntry->entry.name);
    retEntry->entry.inode = free_inode;

    insert_entry(parent, retEntry, inode_entry.mode);

    return EXT2_SUCCESS;
}


/******************************************************************************/
/*																			  */
/*								init function 								  */
/*																			  */
/******************************************************************************/
int fill_super_block(EXT2_SUPER_BLOCK *sb) {
    ZeroMemory(sb, sizeof(EXT2_SUPER_BLOCK));
    sb->max_inode_count = NUMBER_OF_INODES;
    sb->first_data_block_each_group = 1 + BLOCK_NUM_GDT_PER_GROUP + 1 + 1 +
                                      ((INODES_PER_GROUP * INODE_SIZE + (MAX_BLOCK_SIZE - 1)) /
                                       MAX_BLOCK_SIZE);
    sb->block_count = NUMBER_OF_BLOCK;
    sb->reserved_block_count = sb->block_count / 100 * 5;
    sb->free_block_count = NUMBER_OF_BLOCK - (sb->first_data_block_each_group) - 1;
    sb->free_inode_count = NUMBER_OF_INODES - 11;
    sb->first_data_block = SUPER_BLOCK_BASE;
    sb->log_block_size = 0;
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

int fill_descriptor_table(EXT2_GROUP_DESCRIPTOR_TABLE *gdb, EXT2_SUPER_BLOCK *sb) {
    EXT2_GROUP_DESCRIPTOR gd;
    int i;

    ZeroMemory(gdb, sizeof(EXT2_GROUP_DESCRIPTOR_TABLE));
    ZeroMemory(&gd, sizeof(EXT2_GROUP_DESCRIPTOR));

    for (i = 0; i < NUMBER_OF_GROUPS; i++) {
        gd.start_block_of_block_bitmap = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP * i);
        gd.start_block_of_inode_bitmap = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP * i) + 1;
        gd.start_block_of_inode_table = BLOCK_BITMAP_BASE + (BLOCKS_PER_GROUP * i) + 1 + 1;
        gd.free_blocks_count = (sb->free_block_count / NUMBER_OF_GROUPS);
        gd.free_inodes_count = (((sb->free_inode_count) + 10) / NUMBER_OF_GROUPS) * 2;
        gd.directories_count = 0;
        memcpy(&gdb->group_descriptor[i], &gd, sizeof(EXT2_GROUP_DESCRIPTOR));
    }
    return EXT2_SUCCESS;
}

int init_super_block(DISK_OPERATIONS *disk, EXT2_SUPER_BLOCK *sb, UINT32 group_number) {
    BYTE block[MAX_BLOCK_SIZE];
    ZeroMemory(block, sizeof(block));
    memcpy(block, sb, sizeof(block));
    block_write(disk, SUPER_BLOCK_BASE + (group_number * BLOCKS_PER_GROUP), block);
    ZeroMemory(block, sizeof(block));
    block_read(disk, SUPER_BLOCK_BASE + (group_number * BLOCKS_PER_GROUP), block);

    return EXT2_SUCCESS;
}

int init_gdt(DISK_OPERATIONS *disk, EXT2_GROUP_DESCRIPTOR_TABLE *gdt, UINT32 group_number) {
    EXT2_GROUP_DESCRIPTOR gd[NUMBER_OF_GROUPS];
    BYTE block[MAX_BLOCK_SIZE];
    int gdt_block_num;
    EXT2_GROUP_DESCRIPTOR_TABLE *gdt_read = gdt;
    for (gdt_block_num = 0; gdt_block_num < BLOCK_NUM_GDT_PER_GROUP; gdt_block_num++) {
        ZeroMemory(block, sizeof(block));
        memcpy(block, gdt_read + (gdt_block_num * MAX_BLOCK_SIZE), sizeof(block));
        block_write(disk, GDT_BASE + (group_number * BLOCKS_PER_GROUP) + gdt_block_num, block);
    }
    return EXT2_SUCCESS;
}

int init_block_bitmap(DISK_OPERATIONS *disk, UINT32 group_number) {
    BYTE block[MAX_BLOCK_SIZE];
    ZeroMemory(block, sizeof(block));
    int i;
    for (i = 0; i < DATA_BLOCK_BASE; i++)
        set_bit(i, block);
    block_write(disk, BLOCK_BITMAP_BASE + (group_number * BLOCKS_PER_GROUP), block);
    return EXT2_SUCCESS;
}

int init_inode_bitmap(DISK_OPERATIONS *disk, UINT32 group_number) {
    BYTE block[MAX_BLOCK_SIZE];
    ZeroMemory(block, sizeof(block));
    int i;
    for (i = 0; i < 10; i++)
        set_bit(i, block);
    block_write(disk, INODE_BITMAP_BASE + (group_number * BLOCKS_PER_GROUP), block);
    return EXT2_SUCCESS;
}

int init_data_block(DISK_OPERATIONS *disk, UINT32 group_number) {
    BYTE block[MAX_BLOCK_SIZE];
    ZeroMemory(block, sizeof(block));
    block_write(disk, DATA_BLOCK_BASE + (group_number * BLOCKS_PER_GROUP), block);
    return EXT2_SUCCESS;
}

int create_root(DISK_OPERATIONS *disk, EXT2_SUPER_BLOCK *sb) {
    BYTE block[MAX_BLOCK_SIZE];
    SECTOR rootBlock = 0;
    EXT2_DIR_ENTRY *entry;
    EXT2_GROUP_DESCRIPTOR *gd;
    EXT2_SUPER_BLOCK *sb_read;
    QWORD block_num_per_group = BLOCKS_PER_GROUP;
    INODE ip;
    int gi;

    ZeroMemory(block, MAX_BLOCK_SIZE);
    entry = (EXT2_DIR_ENTRY *) block;

    memset(entry->name, 0x20, MAX_ENTRY_NAME_LENGTH);
    entry->name[0] = '.';
    entry->name[1] = '.';
    entry->inode = NUM_OF_ROOT_INODE;
    entry->name_len = 2;

    entry++;
    memset(entry->name, 0x20, 11);
    entry->name[0] = '.';
    entry->inode = NUM_OF_ROOT_INODE;
    entry->name_len = 1;

    entry++;
    entry->name[0] = DIR_ENTRY_NO_MORE;

    rootBlock = 1 + sb->first_data_block_each_group;

    block_write(disk, rootBlock, block);

    sb_read = (EXT2_SUPER_BLOCK *) block;
    for (gi = 0; gi < NUMBER_OF_GROUPS; gi++) {
        block_read(disk, block_num_per_group * gi + SUPER_BLOCK_BASE, block);
        sb_read->free_block_count--;
        block_write(disk, block_num_per_group * gi + SUPER_BLOCK_BASE, block);
    }

    gd = (EXT2_GROUP_DESCRIPTOR *) block;
    block_read(disk, GDT_BASE, block);

    gd->free_blocks_count--;
    gd->directories_count = 1;
    for (gi = 0; gi < NUMBER_OF_GROUPS; gi++)
        block_write(disk, block_num_per_group * gi + GDT_BASE, block);

    block_read(disk, BLOCK_BITMAP_BASE, block);
    set_bit(rootBlock, block);
    block_write(disk, BLOCK_BITMAP_BASE, block);

    ZeroMemory(block, MAX_BLOCK_SIZE);
    ip.mode = 0x1FF | 0x4000;
    ip.size = 0;
    ip.blocks = 1;
    ip.block[0] = DATA_BLOCK_BASE;
    memcpy(block + sizeof(INODE), &ip, sizeof(INODE));
    block_write(disk, INODE_TABLE_BASE, block);

    return EXT2_SUCCESS;
}


/******************************************************************************/
/*																			  */
/*								read function 								  */
/*																			  */
/******************************************************************************/
int read_superblock(EXT2_FILESYSTEM *fs, EXT2_NODE *root) {
    INT result;
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_DIR_ENTRY entry;
    EXT2_DIR_ENTRY_LOCATION location;
    char name[MAX_NAME_LENGTH];

    if (fs == NULL || fs->disk == NULL) {
        WARNING("DISK OPERATIONS : %p\nEXT2_FILESYSTEM : %p\n", fs, fs->disk);
        return EXT2_ERROR;
    }
    if (block_read(fs->disk, SUPER_BLOCK_BASE, block))
        return EXT2_ERROR;

    if (validate_sb(block))
        return EXT2_ERROR;

    memcpy(&fs->sb, block, sizeof(block));

    ZeroMemory(block, sizeof(block));
    block_read(fs->disk, GDT_BASE, block);
    memcpy(&fs->gd, block, sizeof(EXT2_GROUP_DESCRIPTOR));

    ZeroMemory(block, sizeof(MAX_BLOCK_SIZE));
    if (read_root_block(fs, block))
        return EXT2_ERROR;
    ZeroMemory(root, sizeof(EXT2_NODE));
    memcpy(&root->entry, block, sizeof(EXT2_DIR_ENTRY));

    memset(name, SPACE_CHAR, sizeof(name));
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

int read_root_block(EXT2_FILESYSTEM *fs, BYTE *block) {
    UINT32 inode = 2;
    INODE inodeBuffer;
    SECTOR rootBlock;
    get_inode(fs, inode, &inodeBuffer);
    rootBlock = inodeBuffer.block[0];
    return block_read(fs->disk, rootBlock, block);
}

int read_dir_from_block(EXT2_FILESYSTEM *fs, BYTE *block, EXT2_NODE_ADD adder, void *list) {
    EXT2_DIR_ENTRY *dir_entry;
    EXT2_NODE node;
    dir_entry = (EXT2_DIR_ENTRY *) block;
    UINT i = 0;

    while (dir_entry[i].name[0] != DIR_ENTRY_NO_MORE) {
        if ((dir_entry[i].name[0] != '.') && (dir_entry[i].name[0] != DIR_ENTRY_FREE)) {
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
int get_inode(EXT2_FILESYSTEM *fs, UINT32 inode_num, INODE *inodeBuffer) {
    BYTE block[MAX_BLOCK_SIZE];
    inode_num--;
    UINT32 group_number = inode_num / INODES_PER_GROUP;
    UINT32 group_inode_offset = inode_num % INODES_PER_GROUP;
    UINT32 block_number = group_inode_offset / INODES_PER_BLOCK;
    UINT32 block_inode_offset = group_inode_offset % INODES_PER_BLOCK;

    block_read(fs->disk, INODE_TABLE_BASE + (group_number * BLOCKS_PER_GROUP) + block_number,
               block);
    memcpy(inodeBuffer, block + (INODE_SIZE * (block_inode_offset)), sizeof(INODE));
}

int lookup_entry(EXT2_FILESYSTEM *fs, int inode, char *name, EXT2_NODE *retEntry) {

    SECTOR block[MAX_BLOCK_SIZE];
    INODE inodeBuffer;
    EXT2_DIR_ENTRY *entry;
    INT32 lastEntry = MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY) - 1;
    UINT32 i, result, number;
    UINT32 begin = 0;

    get_inode(fs, inode, &inodeBuffer);

    for (i = 0; i < inodeBuffer.blocks; i++) {
        block_read(fs->disk, inodeBuffer.block[i], block);
        entry = (EXT2_DIR_ENTRY *) block;
        retEntry->location.block = inodeBuffer.block[i];
        result = find_entry_at_block(block, name, begin, lastEntry, &number);

        switch (result) {
            case -2: {
                return EXT2_ERROR;
            }
            case -1: {
                continue;
            }
            case 0: {
                memcpy(&retEntry->entry, &entry[number],
                       sizeof(EXT2_DIR_ENTRY));
                retEntry->location.group = GET_INODE_GROUP(entry[number].inode);
                retEntry->location.block = inodeBuffer.block[i];
                retEntry->location.offset = number;
                retEntry->fs = fs;
                return EXT2_SUCCESS;
            }
        }
    }
    return EXT2_ERROR;
}

int find_entry_at_block(BYTE *block, char *formattedName, UINT32 begin, UINT32 last, UINT32 *number) {
    UINT32 i;
    EXT2_DIR_ENTRY *entry = (EXT2_DIR_ENTRY *) block;

    for (i = begin; i < last; i++) {
        if (formattedName == NULL) {
            if (entry->name[0] != DIR_ENTRY_NO_MORE && entry->name[0] != DIR_ENTRY_FREE && (entry->name[0] != '.')) {
                *number = i;
                return EXT2_SUCCESS;
            }
        } else {
            if (memcmp(entry->name, formattedName, MAX_ENTRY_NAME_LENGTH) == 0) {
                *number = i;
                return EXT2_SUCCESS;
            }
            if ((formattedName[0] == DIR_ENTRY_FREE || formattedName[0] == DIR_ENTRY_NO_MORE) &&
                (formattedName[0] == entry->name[0])) {
                *number = i;
                return EXT2_SUCCESS;
            }
        }
        if (entry->name[0] == DIR_ENTRY_NO_MORE) {
            *number = i;
            return -2;
        }
        entry++;
    }
    *number = i;
    return EXT2_ERROR;
}

UINT32 get_free_inode_number(EXT2_FILESYSTEM *fs) {
    EXT2_INODE_BITMAP i_bitmap;
    EXT2_BITSET inodeset;
    int i, j;
    unsigned char k = 0x80;

    block_read(fs->disk, (fs->sb.block_group_number * BLOCKS_PER_GROUP) + INODE_BITMAP_BASE, &i_bitmap);

    for (i = 0; i < fs->sb.inode_per_group && i < MAX_BLOCK_SIZE / 4; i++) {
        if (i_bitmap.inode_bitmap[i] != 0xff) {
            for (j = 0; j < 8; j++) {
                if (!(i_bitmap.inode_bitmap[i] & (k >> j))) {
                    inodeset.bit_num = 8 * i + j;
                    inodeset.index_num = i;
                    set_inode_bitmap(fs, &i_bitmap, inodeset);
                    return inodeset.bit_num + 1;
                }
            }
        }
    }
    return 0;
}

UINT32 get_free_block_number(EXT2_FILESYSTEM *fs) {
    EXT2_BLOCK_BITMAP b_bitmap;
    EXT2_BITSET blockset;
    int i, j;
    unsigned char k = 0x80;

    block_read(fs->disk, (fs->sb.block_group_number * BLOCKS_PER_GROUP) + BLOCK_BITMAP_BASE, &b_bitmap);


    for (i = 0; i < fs->sb.block_per_group && i < MAX_BLOCK_SIZE / 4; i++) {
        if (b_bitmap.block_bitmap[i] != 0xff) {
            for (j = 0; j < 8; j++) {
                if (!(b_bitmap.block_bitmap[i] & (k >> j))) {
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

int get_data_block_at_inode(EXT2_FILESYSTEM *fs, INODE inode, UINT32 number) {
    return inode.block[number];
}


/******************************************************************************/
/*																			  */
/*								set function 								  */
/*																			  */
/******************************************************************************/
int insert_entry(EXT2_NODE *parent, EXT2_NODE *newEntry, UINT16 fileType) {

    EXT2_DIR_ENTRY_LOCATION begin;
    EXT2_NODE entry;
    char entryName[2] = {0,};
    INODE inodeBuffer;
    INT32 free_block_num, free_inode_num;
    entryName[0] = DIR_ENTRY_FREE;
    if (lookup_entry(parent->fs, parent->entry.inode, entryName, &entry) == EXT2_SUCCESS) {

        set_entry(parent->fs, &entry.location, &newEntry->entry);
        newEntry->location = entry.location;
    } else {
        entryName[0] = DIR_ENTRY_NO_MORE;
        if (lookup_entry(parent->fs, parent->entry.inode, entryName, &entry) == EXT2_ERROR)
            return EXT2_ERROR;

        set_entry(parent->fs, &entry.location, &newEntry->entry);
        newEntry->location = entry.location;
        entry.location.offset++;

        if (entry.location.offset == MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY)) {
            get_inode(parent->fs, parent->entry.inode, &inodeBuffer);
            free_block_num = get_free_block_number(parent->fs);
            inodeBuffer.block[inodeBuffer.blocks - 1] = free_block_num;
            entry.location.block = free_block_num;
            entry.location.offset = 0;

        }
        set_entry(parent->fs, &entry.location, &entry.entry);
    }
    return EXT2_SUCCESS;
}

int insert_inode_table(EXT2_NODE *parent, INODE *inode_entry, int free_inode) {
    DISK_OPERATIONS *disk;
    disk = parent->fs->disk;
    BYTE block[MAX_BLOCK_SIZE];
    INODE *inode;
    free_inode--;

    UINT32 group_num = free_inode / INODES_PER_GROUP;
    UINT32 block_num = (free_inode % INODES_PER_GROUP) / INODES_PER_BLOCK;
    UINT32 block_offset = (free_inode % INODES_PER_GROUP) % INODES_PER_BLOCK;

    block_read(disk, (group_num * BLOCKS_PER_GROUP) + INODE_TABLE_BASE + block_num, block);

    inode = (INODE *) block;
    inode = inode + block_offset;
    *inode = *inode_entry;

    block_write(disk, (group_num * BLOCKS_PER_GROUP) + INODE_TABLE_BASE + block_num, block);

    return EXT2_SUCCESS;
}

int set_sb_free_block_count(EXT2_NODE *node, int num) {
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_SUPER_BLOCK *sb;
    DISK_OPERATIONS *disk = node->fs->disk;
    block_read(disk, SUPER_BLOCK_BASE, block);
    sb = (EXT2_SUPER_BLOCK *) block;

    if (num > 0)
        sb->free_block_count += num;
    else if (sb->free_block_count >= -num) {
        sb->free_block_count += num;
    } else {
        PRINTF("No more free_block exist\n");
        return EXT2_ERROR;
    }
    block_write(disk, SUPER_BLOCK_BASE, block);
}

int set_sb_free_inode_count(EXT2_NODE *node, int num) {
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_SUPER_BLOCK *sb;
    DISK_OPERATIONS *disk = node->fs->disk;

    block_read(disk, SUPER_BLOCK_BASE, block);
    sb = (EXT2_SUPER_BLOCK *) block;
    if (num > 0)
        sb->free_inode_count += num;
    else if (sb->free_inode_count >= -num) {
        sb->free_inode_count += num;
    } else {
        PRINTF("No more free_inode exist\n");
        return EXT2_ERROR;
    }
    block_write(disk, SUPER_BLOCK_BASE, block);
}

int set_gd_free_block_count(EXT2_NODE *node, UINT32 free_data_block, int num) {
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_GROUP_DESCRIPTOR *gd;
    DISK_OPERATIONS *disk = node->fs->disk;
    UINT32 group_num = GET_DATA_GROUP(free_data_block);
    UINT32 gd_block_num = group_num * sizeof(EXT2_GROUP_DESCRIPTOR) / MAX_BLOCK_SIZE;
    UINT32 gd_block_offset = group_num % (MAX_BLOCK_SIZE / sizeof(EXT2_GROUP_DESCRIPTOR));

    block_read(disk, GDT_BASE + gd_block_num, block);
    gd = (EXT2_GROUP_DESCRIPTOR *) block;
    gd += gd_block_offset;
    if (num > 0)
        gd->free_blocks_count += num;
    else if (gd->free_blocks_count >= -num) {
        gd->free_blocks_count += num;
    } else {
        PRINTF("No more free_block exist\n");
        return EXT2_ERROR;
    }
    block_write(disk, GDT_BASE + gd_block_num, block);
}

int set_gd_free_inode_count(EXT2_NODE *node, UINT32 free_inode, int num) {
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_GROUP_DESCRIPTOR *gd;
    DISK_OPERATIONS *disk = node->fs->disk;
    UINT32 group_num = GET_INODE_GROUP(free_inode);
    UINT32 gd_block_num = group_num * sizeof(EXT2_GROUP_DESCRIPTOR) / MAX_BLOCK_SIZE;
    UINT32 gd_block_offset = group_num % (MAX_BLOCK_SIZE / sizeof(EXT2_GROUP_DESCRIPTOR));

    block_read(disk, GDT_BASE + gd_block_num, block);
    gd = (EXT2_GROUP_DESCRIPTOR *) block;
    gd += gd_block_offset;

    if (num > 0)
        gd->free_inodes_count += num;
    else if (gd->free_blocks_count >= -num)
        gd->free_inodes_count += num;
    else {
        PRINTF("No more free_block exist\n");
        return EXT2_ERROR;
    }
    block_write(disk, GDT_BASE + gd_block_num, block);
}

int set_entry(EXT2_FILESYSTEM *fs, EXT2_DIR_ENTRY_LOCATION *location, EXT2_DIR_ENTRY *value) {
    BYTE block[MAX_BLOCK_SIZE];
    EXT2_DIR_ENTRY *entry;
    int result;
    result = block_read(fs->disk, location->block, block);
    entry = (EXT2_DIR_ENTRY *) block;
    entry[location->offset] = *value;
    result = block_write(fs->disk, location->block, block);

    return result;
}

int set_inode_bitmap(EXT2_FILESYSTEM *fs, EXT2_INODE_BITMAP *i_bitmap, EXT2_BITSET bitset) {
    DISK_OPERATIONS *disk = fs->disk;
    i_bitmap->inode_bitmap[bitset.index_num] ^= (0x80 >> ((bitset.bit_num % 8)));
    block_write(disk, (fs->sb.block_group_number * BLOCKS_PER_GROUP) + INODE_BITMAP_BASE, i_bitmap);
}

int set_block_bitmap(EXT2_FILESYSTEM *fs, EXT2_BLOCK_BITMAP *b_bitmap, EXT2_BITSET bitset) {
    DISK_OPERATIONS *disk = fs->disk;
    b_bitmap->block_bitmap[bitset.index_num] ^= (0x80 >> ((bitset.bit_num % 8)));
    block_write(disk, (fs->sb.block_group_number * BLOCKS_PER_GROUP) + BLOCK_BITMAP_BASE, b_bitmap);
}

void
fill_data_block(DISK_OPERATIONS *disk, INODE *inodetemp, unsigned long length, unsigned long offset, const char *buffer,
                UINT32 block_num) {
    BYTE block[MAX_BLOCK_SIZE];
    UINT32 copy_num = MAX_BLOCK_SIZE / SHELL_BUFFER_SIZE;
    int i, j;
    unsigned long lesslength = length;
    unsigned long cur_offset = offset;


    if (block_num != 0) {
        for (i = 0; i < block_num + 1; i++) {
            ZeroMemory(block, sizeof(block));
            cur_offset = 0;
            for (j = 0; j < copy_num; j++) {
                memcpy(&block[cur_offset], buffer, SHELL_BUFFER_SIZE);
                cur_offset += SHELL_BUFFER_SIZE;
            }
            block_write(disk, inodetemp->block[i], block);
            lesslength -= MAX_BLOCK_SIZE;
        }
    } else {
        ZeroMemory(block, sizeof(block));
        cur_offset = 0;
        for (i = 0; i < ((lesslength + SHELL_BUFFER_SIZE - 1) / SHELL_BUFFER_SIZE); i++) {
            memcpy(&block[cur_offset], buffer, SHELL_BUFFER_SIZE);
            cur_offset += SHELL_BUFFER_SIZE;
        }
        block_write(disk, inodetemp->block[block_num], block);
    }
}


/******************************************************************************/
/*																			  */
/*							utility function 								  */
/*																			  */
/******************************************************************************/
int format_name(EXT2_FILESYSTEM *fs, char *name) {
    UINT32 i, length;
    UINT32 extender = 0, nameLength = 0;
    UINT32 extenderCurrent = 8;
    char regularName[MAX_ENTRY_NAME_LENGTH];

    memset(regularName, 0x20, sizeof(regularName));
    length = strlen(name);

    if (strncmp(name, "..", 2) == 0) {
        memcpy(name, "..         ", 11);
        return EXT2_SUCCESS;
    } else if (strncmp(name, ".", 1) == 0) {
        memcpy(name, ".          ", 11);
        return EXT2_SUCCESS;
    } else {
        upper_string(name, MAX_ENTRY_NAME_LENGTH);
        for (i = 0; i < length; i++) {
            if (name[i] != '.' && !isdigit(name[i]) && !isalpha(name[i]))
                return EXT2_ERROR;

            if (name[i] == '.') {
                if (extender)
                    return EXT2_ERROR;
                extender = 1;
            } else if (isdigit(name[i]) || isalpha(name[i])) {
                if (extender)
                    regularName[extenderCurrent++] = name[i];
                else
                    regularName[nameLength++] = name[i];
            } else
                return EXT2_ERROR;
        }

        if (nameLength > 8 || nameLength == 0 || extenderCurrent > 11)
            return EXT2_ERROR;
    }

    memcpy(name, regularName, sizeof(regularName));
    return EXT2_SUCCESS;
}

int block_write(DISK_OPERATIONS *this, SECTOR block, void *data) {
    int i;
    int result;
    int sectorNum = block * SECTORS_PER_BLOCK;

    for (i = 0; i < SECTORS_PER_BLOCK; i++) {
        result = disksim_write(this, sectorNum + i, data + (i * MAX_SECTOR_SIZE));
    }
    return result;
}

int block_read(DISK_OPERATIONS *this, SECTOR block, void *data) {
    int i;
    int result;
    int sectorNum = block * SECTORS_PER_BLOCK;

    for (i = 0; i < SECTORS_PER_BLOCK; i++) {
        result = disksim_read(this, sectorNum + i, data + (i * MAX_SECTOR_SIZE));
    }
    return result;
}

int set_bit(SECTOR number, BYTE *bitmap) {
    BYTE value;
    SECTOR byte = number / 8;
    SECTOR offset = number % 8;

    switch (offset) {
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

int set_zero_bit(SECTOR number, BYTE *bitmap) {
    BYTE value;
    SECTOR byte = number / 8;
    SECTOR offset = number % 8;

    switch (offset) {
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

int dump_memory(DISK_OPERATIONS *disk, int block) {

    BYTE dump[1024];
    ZeroMemory(dump, sizeof(dump));
    block_read(disk, block, dump);
    int i;
    for (i = 0; i < 64; i++) {
        int j;
        printf("%p\t ", &dump[i * 16]);
        for (j = (16 * i); j < (16 * i) + 16; j++) {
            printf("%02x  ", dump[j]);
        }
        printf("\n");
    }
    return EXT2_SUCCESS;
}

int my_strnlen(char *src) {
    int num = 0;
    while (*src && *src != 0x20) {
        *src++;
        num++;
    }
    return num;
}

void upper_string(char *str, int length) {
    while (*str && length-- > 0) {
        *str = toupper(*str);
        str++;
    }
}

int validate_sb(void *block) {
    EXT2_SUPER_BLOCK *sb = (EXT2_SUPER_BLOCK *) block;

    if (!(sb->magic_signature == 0xEF53))
        return EXT2_ERROR;

    return EXT2_SUCCESS;
}