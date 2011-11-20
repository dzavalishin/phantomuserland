/*!     \file include/kernel/ext2.h
 *      \brief ext2 file system header.
 *      \author
 *              Filippo Brogi
 *      \note Copyright (&copy;) 2003
 *              Filippo Brogi
 *      \date Last update:
 *              2003-09-30 by Andrea Righi
 *                      init_ext2() now returns a boolean value.\n
 */

#ifndef EXT2_H
#define EXT2_H

//#include <const.h>

typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned char byte;

//typedef int bool;

#define TRUE 1
#define FALSE 0

/** \ingroup FileSystem
 *  \defgroup FSext2 Ext2
 *  The ext2 file system.
 *  @{
 */

// dimensione blocco del disco
#define SIZE_SEC 512

// numero magico ext2
#define N_EXT2_NUMERO_MAGICO 0xEF53 //for normal versione of ext2
#define P_EXT2_NUMERO_MAGICO 0xEF51 //for versions of ext2fs prior to 0.2b.

// costanti errore ext2
#define EXT2_ERRORS_CONTINUE 1 // continua come se niente fosse
#define EXT2_ERRORS_RO 2 // rimonta a sola lettura
#define EXT3_ERRORS_PANIC 3 // causa un panic nel kernel

// valori EXT2_OS
#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_MASIX 2
#define EXT2_FREEBSD 3
#define EXT2_OS_LITES4 4

// livelli di revisione
#define EXT2_GOOD_OLD_REV 0 //original format
#define EXT2DYNAMIC_REV 1 // formato v2 con dimensione inode dinamica

// valori EXT2_*_INO
#define EXT2_BAD_INO 0x01 //blocco inode danneggiato
#define EXT2_ROOT_INO 0x02 // inode directory radice
#define EXT2_ACL_IDX_INO 0x03 //ACL index inode
#define EXT2_ACL_DARA_INO 0x04 //ACL data inode
#define EXT2_BOOTLOADER INO 0x05 //boot loader inode
#define EXT2_UNDEL_DIR_INO 0x06 // inode directory ripristinata

// valori EXT2_S_I

// ------------ file format ------------
#define MODE_MASK 0xF000 // format mask
#define MODE_SOCK 0xC000 // socket
#define MODE_LINK 0xA000 // symbolic link
#define MODE_FILE 0x8000 // regular file
#define MODE_BDEV 0x6000 // block device
#define MODE_DIR 0x4000 // directory
#define MODE_CDEV 0x2000 // character device
#define MODE_FIFO 0x1000 // fifo

// ------------ access rights ------------
#define EXT2_S_ISUID 0x0800 // SUID
#define EXT2_S_ISGID 0x0400 // SGID
#define EXT2_S_ISVTX 0x0200 // sticky bit
#define EXT2_S_IRWXU 0x01C0 // user access rights mask
#define EXT2_S_IRUSR 0x0100 // read
#define EXT2_S_IWUSR 0x0080 // write
#define EXT2_S_IXUSR 0x0040 // execute
#define EXT2_S_IRWXG 0x0038 // group access right mask
#define EXT2_S_IRGRP 0x0020 // read
#define EXT2_S_IWGRP 0x0010 // write
#define EXT2_S_IXGRP 0x0008 // execute
#define EXT2_S_IRWXO 0x0007 // others access rights mask
#define EXT2_S_IROTH 0x0004 // read
#define EXT2_S_IWOTH 0x0003 // write
#define EXT2_S_IXOTH 0x0001 // execute


// valori EXT2_FT
//------------ tipo file ---------------
#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7
#define EXT2_FT_MAX 8


#define DIM_SUPER_BLOCK 1024
#define START_SUPER_BLOCK 1024
struct super_block
{
    dword s_inodes_count; //numero totale inode liberi e utilizzati
    dword s_blocks_count; //numero totale di blocchi liberi e utilizzati
    dword s_r_blocks_count; //numero totale di blocchi riservati al super user
    dword s_free_blocks_count; //numero di blocchi liberi compresi quelli riservati
    dword s_free_inodes_count; //numero totale di inode liberi

    dword s_first_data_block; /* id of the block containing the structure of the sb
    generally this value '0 for file systems with a size of
    block greater than 1KB. The super block always starts with 1024 bytes of disk
    which generally coincides with the first byte of the third sector */

    dword s_log_block_size; /* la dimensione del blocco si calcola come il numero
    di bit che si ottiene shiftando 1024. Questo valore puo` essere solo positivo;
    block size = 1024 << s_log_block_size; */
    dword s_log_frag_size; /* la dimensione del frammento si calcola come il numero
    di bit da shiftare dal valore 1024
    if (positive)
    fragment size = 1024 << s_log_frag_size;
    else
    fragment size = 1024 >> -s_log_frag_size; */
    dword s_blocks_per_group; //numero totale blocchi del gruppo
    dword s_frags_per_group; //numero totale frammenti per gruppo
    dword s_inodes_per_group; //numero totale di inodes per gruppo
    dword s_mtime; // data ultimo montaggio del file system
    dword s_wtime; // ultimo accesso in scrittura al file system
    word s_mnt_count; // numero di volte che e' stato montato dall'ultima volta
    // in cui e' stata completamente verificata
    word s_max_mnt_count; //numero massimo di volte che un file system puo` essere
    //montato prima che sia eseguito un check completo
    word s_magic; // numero magico

    word s_state; /* state of the mounted file system. When the fs and 'was mounted
    state and 'place to EXT2_ERROR_FS. When the file system is not 'yet
    value can be mounted EXT2_VALID_FS or EXT2_ERROR_FS if not 'status
    completely disassembled */

    word s_errors; //cosa fare in caso di errore
    word s_minor_rev_level; //
    dword s_lastcheck; // ultimo check del file system
    dword s_checkinterval; // massimo tempo di intervallamento tra due check
    dword s_creator_os; // identificativo so creatore del file system
    dword s_rev_level; // valore livello di revisione
    word s_def_resuid; // id user default per i blocchi riservati
    word s_def_resgid; // id group default per i blocchi riservati

    dword s_first_ino; /* indice del primo inode utilizzabile per file standard.
    nella revisione non dinamica del file system questo valore e' fissato a 11. Con
    quella dinamica e' possibile modificare questo valore*/
    word s_inode_size; /* dimensione  della struttura inode. Nel caso non
    dinamico questo valore e' 128*/
    word s_block_group_nr; // numero dei gruppi nel superblocco
    dword s_feature_compact; //
    dword s_feature_incompact; //
    dword s_feature_ro_compact; //
    byte s_uuid[16]; // id del volume 128 bit
    word s_volume_name; //
    byte s_last_mounted[8]; // path directory dove e' stato montato il fs
    dword s_algo_bitmap; /* usato da algoritmi di compressione per determinare i
    metodi utilizzati */
    byte s_reserved[886]; // riservati al sistema operativo
};

/*
The descriptor of group and 'an array of structure group_desc each
 which defines a group of blocks, giving the location of the table
 inodes, bitmap blocks, and inodes, and other information yet. In
 General and 'allocated consecutively to the disk block containing the super
 block
 */


struct group_descriptor{
    dword bg_block_bitmap; // id primo blocco della bitmap dei blocchi del gruppo
    dword bg_inode_bitmap; // id primo blocco della bitmap degli inode
    dword bg_inode_table; //primo blocco della tabella degli inode
    word bg_free_blocks_count; //numero totale di blocchi liberi
    word bg_free_inodes_count; // numero totale inode liberi
    word bg_used_dirs_count; // numero inode allocati nelle directory
    word bg_pad; // valore usato per il padding della struttura
    dword bg_reserved[3]; // valori riservati per future implementazioni
};


/*
The "block bitmap" and 'normally located on the first block or second block
 if you have the backup of the superblock. Its official location
 you can get by reading the descriptor in bg_block_bitmap group. Each
 bit represents the current state of the block, 1 being used, while 0 indicates
 Free / available.

 L ' "inode bitmap" works the same way the bitmap of the blocks. This
 bitmap is determined from bg_inode_bitmap. When you create the table
 all the reserved inode inode are marked as used.

 The inode table and 'used to keep track of each file: lease
 size, type, access rights are all stored in the inode. Names
 Files are not stored here, in the inode table all files are
 referenced with their inode number. The inode table and 'referenced
 by bg_inode_table and contains s_inodes_per_group
 */



struct i_node{
    word i_mode; // formato del file e diritti di accesso
    word i_uid; // user id associato col file
    dword i_size; // dimensione in byte del file
    dword i_atime; // ultimo accesso in secondi a partire dal 1/1/1970
    dword i_ctime; // data di creazione in secondi dal 1/1/1970
    dword i_mtime; // data ultima modifica in secondi dal 1/1/1970
    dword i_dtime; // data della cancellazione del file a partire dal 1/1/1970
    word i_gid; // gruppo che ha accesso al file
    word i_links_count; // numero dei riferimenti all'inode

    dword i_blocks; /* amount of blocks associated with the file your currently
    used and those that will be used in case of an increase in
    file size. Qusto In case the size of blocks and '512 kB and
    that specified in the superblock */

    dword i_flags; // comportamento del file system quando accede ai dati
    dword i_osd1; // valore dipendendente dal SO

    dword i_block[15]; /* array used to identify the disk blocks in
    which is stored the file. The first 12 elements are used to direct
    directly the data blocks associated with files, the 13-th and 'used to
    addressing indiriretto single on 14-th for indirect addressing
    double and 15 th for the triple */

    dword i_generation; // indica versione del file (usato da NFS)
    dword i_file_acl; //numero el blocco contenenti gli attributi estesi
    dword i_dir_acl; // indica la high size del file
    dword i_faddr; // locazione dell'ultimo frammento del file
    /* inode osd2 structure Linux */
    byte l_i_frag; //numero frammento
    byte l_i_fsize; // dimensione frammento
    word reserved1; // reserved
    word l_i_uid_high; // bit dell'user id
    word l_i_gid_high; // bit del group id
    dword reserved2; // reserved
};

/*
 Directories are stored as files and can be identified by looking at the
 value of the inode i_mode and verifying that it is equal to
 EXT2_S_IFDIR. The root and 'always stored in second position
 of the inode table. Each subdirectory you can get watching
 contents of the directory tree root.
 */

struct dir_ff{
    dword inode; // numero entry del file. 0 indica che non e' usata
    word rec_len; // move to the next element in the current directory
    byte name_len; // many characters contain the name
    byte file_type; // tipo del file
    char name[1]; // nome dell'entry
};

/*

 Use the standard format of linked lists for directories may
 become very slow when the number of files starts to grow. `So
 incrementre for performance using a hash index that helps
 enhance the performance of research

 */

/*
Index structure

 The root of the tree index is located in block 0 of the file. Space for
 second tree level indicated (for file system with 4KB block)
 is located in blocks 1 to 511. The blocks of the directory tree are leaf
 located from block 512, so the tail of the file directory
 looks like a standard directory tree and can be processed with EXT2
 ext2_readdir. For directory tree with less than 90K file there is a hole running from
 block 1 to block 511, so an empty directory has just two blocks, although
 its size is approximately 2 mega list of the directory tree.
 */

// --- Prototypes ----------------------------------------------------- //

//bool init_ext2(void);
//char *pwd_ext2();
//void ls_ext2(void);
//void cd_ext2(char *param);
//void cat_ext2(char *stringa);

/** @} */ // end of FSext2

#endif
