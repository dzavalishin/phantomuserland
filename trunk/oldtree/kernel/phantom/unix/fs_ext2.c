/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Ext2 FS
 *
 *
**/

#if HAVE_UNIX


#define DEBUG_MSG_PREFIX "ext2fs"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <unix/uufile.h>
//#include <unix/uuprocess.h>
#include <malloc.h>
#include <string.h>
#include <phantom_libc.h>
#include <sys/libkern.h>

#include "unix/fs_ext2.h"

// struttura tabella inode
struct i_node_tab
{
    int 		i_node_n; // numero dell'inode
    struct i_node 	inode; // informazioni generali inode
    // parametri per la gestione tabella inode
    int 		ref;
};


struct e2impl
{
    int				mount_ext2; // Nonzero if impl is active

    int (*ReadSectorsLba)  	(
                                 u_int64_t  	Lba,
                                 int  	  	NumSect,
                                 void *  	Buffer
                                );

    struct super_block *	super;

    int 			dim_inode_table;
    int 			free_inode; //numero elementi della tabella inode liberi

    struct i_node_tab *		inode_table;
    hal_mutex_t                 inode_lock; // inode table modif

    struct group_descriptor  *	group_desc_table;
    hal_mutex_t                 group_lock; // inode table modif

    int                         dim_block; // block size in bytes
    int 			dim_ptr; // dimensione del blocco dati e puntatore
    int 			dim_frag; // dimensione del blocco dati e puntatore

    int 			spb,bpg,gdpb,ipb,sbpos;
    int 			number_of_groups; // numero gruppi di blocchi nel file
    int 			inodes_per_block; // numero inode per blocco
    int 			dir_entries_per_block; // numero directory entries per block

};

typedef struct e2impl e2impl_t;


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static bool		init_ext2(e2impl_t *impl);


static size_t      ext2_read(    struct uufile *f, void *dest, size_t bytes);
static size_t      ext2_write(   struct uufile *f, void *dest, size_t bytes);
//static errno_t     ext2_stat(    struct uufile *f, struct ??);
//static errno_t     ext2_ioctl(   struct uufile *f, struct ??);

static size_t      ext2_getpath( struct uufile *f, void *dest, size_t bytes);

// returns -1 for non-files
static size_t      ext2_getsize( struct uufile *f);

static void *      ext2_copyimpl( void *impl );


static struct uufileops ext2_fops =
{
    .read 	= ext2_read,
    .write 	= ext2_write,

    .getpath 	= ext2_getpath,
    .getsize 	= ext2_getsize,

    .copyimpl   = ext2_copyimpl,

    //.stat       = ext2_stat,
    //.ioctl      = ext2_ioctl,
};



// -----------------------------------------------------------------------
// FS struct
// -----------------------------------------------------------------------


//static uufile_t *  ext2_open(const char *name, int create, int write);
static errno_t     ext2_open(struct uufile *, int create, int write);
static errno_t     ext2_close(struct uufile *);

// Create a file struct for given path
static uufile_t *  ext2_namei(const char *filename);

// Return a file struct for fs root
static uufile_t *  ext2_getRoot();


struct uufs ext2_fs =
{
    .name       = "ext2",
    .open 	= ext2_open,
    .close 	= ext2_close,
    .namei 	= ext2_namei,
    .root 	= ext2_getRoot,
};


static struct uufile ext2_root =
{
    .ops 	= &ext2_fops,
    .pos        = 0,
    .fs         = &ext2_fs,
    .impl       = "/",
};



// -----------------------------------------------------------------------
// FS methods
// -----------------------------------------------------------------------


static errno_t     ext2_open(struct uufile *f, int create, int write)
{
    return 0;
}

static errno_t     ext2_close(struct uufile *f)
{
    if( f->impl )
    {
        free(f->impl);
        f->impl = 0;
    }
    return 0;
}

// Create a file struct for given path
static uufile_t *  ext2_namei(const char *filename)
{
    uufile_t *ret = create_uufile();

    ret->ops = &ext2_fops;
    ret->pos = 0;
    ret->fs = &ext2_fs;
    ret->impl = calloc( 1, sizeof(e2impl_t) );

    return ret;
}

// Return a file struct for fs root
static uufile_t *  ext2_getRoot()
{
    return &ext2_root;
}


// -----------------------------------------------------------------------
// Generic impl
// -----------------------------------------------------------------------

static size_t      ext2_read(    struct uufile *f, void *dest, size_t bytes)
{
    return -1;
}

static size_t      ext2_write(   struct uufile *f, void *dest, size_t bytes)
{
    return -1;
}

//static errno_t     ext2_stat(    struct uufile *f, struct ??);
//static errno_t     ext2_ioctl(   struct uufile *f, struct ??);

static size_t      ext2_getpath( struct uufile *f, void *dest, size_t bytes)
{
    if( bytes == 0 ) return 0;
    // TODO get mountpoint
    // we keep name in impl
    strncpy( dest, f->impl, bytes );
    return strlen(dest);
}

// returns -1 for non-files
static size_t      ext2_getsize( struct uufile *f)
{
    return -1;
}

static void *      ext2_copyimpl( void *impl )
{
    void *dest = calloc( 1, sizeof(e2impl_t) );
    memcpy( dest, impl, sizeof(e2impl_t) );
    return dest;
}

















// -----------------------------------------------------------------------
// Impl
// -----------------------------------------------------------------------




static struct group_descriptor * get_group_desc( e2impl_t *impl, int grp);
static int Inode2Block(e2impl_t *impl, int ino);








static int blkno_to_LBA( e2impl_t *impl, int num_block)
{
    return num_block * impl->spb;
}




// -----------------------------------------------------------------------
// Inodes
// -----------------------------------------------------------------------



// funzione che permette di determinare se e' un fast symbolic link
static bool isFastSymbolicLink(struct i_node* ino)
{
    if ((ino->i_mode & MODE_MASK)==MODE_LINK)
        return TRUE;
    return FALSE;
}

// funzione che permette di verificare se e' un file regolare
static bool isFile(struct i_node* ino)
{
    if ((ino->i_mode & MODE_MASK)==MODE_FILE)
        return TRUE;
    return FALSE;
}

// funzione che permette di verificare se e' un file
static bool isDir(struct i_node* ino)
{
    if ((ino->i_mode & MODE_MASK)==MODE_DIR)
        return TRUE;
    return FALSE;
}


// lettura di un inode dal disco
static bool ReadInode(e2impl_t *impl, int ino, struct i_node* data)
{
    int ino_block = Inode2Block(impl,ino);
    if (!ino_block)
    {
        SHOW_ERROR( 1, "error finding inode block %d", ino );
        return FALSE;
    }

    char * data_block = malloc(impl->dim_block);
    memset( data_block, 0, impl->dim_block);
    //kprintf("\n\rino_block: %u",ino_block);

    int err;

    // TODO : si deve leggere in data_block il blocco del disco che contiene l'inode
    if ( (err = impl->ReadSectorsLba((int64_t)blkno_to_LBA(impl,ino_block), impl->spb, (word *)data_block)) )
    {
        // errore nella lettura del blocco relativo all'inode
        SHOW_ERROR( 1, "error reading inode block %d", ino );
        //ShowIdeErrorMessage(err,TRUE);
        return FALSE;
    }
    // si copia l'inode relativo
    memcpy( data, data_block + ((ino-1)%impl->ipb) * sizeof(struct i_node), sizeof(struct i_node) );
    //stampa_i(data);
    free( data_block );
    return TRUE;
}


//---------------- Cerca elemento LRU -----------------------//
// Called in mutex
static int inode_LRU(e2impl_t *impl)
{
    word lru = 0;
    int i;

    // The item sought and 'the least referenced in the last
    // 16 references in memory, the variable ref and '16-bit
    for( i = 1; i < impl->dim_inode_table; i++)
        if( impl->inode_table[i].ref < impl->inode_table[lru].ref )
            lru = i;

    //kprintf("\n\rci sono passato lru: %u %u",lru,inode_table[lru].ref);
    return lru;
}


//--------------- Cerca inode e restituisce la posizione -------------------//

// ERROR inode is not locked and can be deleted!
static struct i_node *get_inode(e2impl_t *impl, int i_node_number)
{
    int i;
    int pos_inode = -1;
    struct i_node new_inode;

    // controlliamo la validita` dell'inode
    if (!i_node_number || i_node_number > impl->super->s_inodes_count)
    {
        SHOW_ERROR( 1, "Invalid inode number %d", i_node_number );
        return 0;
    }

    //kprintf("\n\r inode_number: %u",i_node_number);

    hal_mutex_lock(&(impl->inode_lock));

    // si cerca se l'inode e' presente nella tabella
    for (i=0; i < (impl->dim_inode_table-impl->free_inode); i++)
        if( impl->inode_table[i].i_node_n == i_node_number )
        {
            // memorizza posizione inode
            pos_inode = i;
            break;
        }

    if (pos_inode == -1)
    {
        // l'elemento non e' presente nella cache dobbiamo reperirlo su disco
        hal_mutex_unlock(&(impl->inode_lock));
        ReadInode( impl, i_node_number, &new_inode );
        hal_mutex_lock(&(impl->inode_lock));
        //stampa_i(&new_inode);

        if( impl->free_inode > 0 ) // ci sono posizioni non utilizzate
        {
            pos_inode = impl->dim_inode_table - impl->free_inode;
            impl->free_inode--; // decremento numero posizioni libere
        }
        else
        {
            pos_inode = inode_LRU(impl);
        }
        impl->inode_table[pos_inode].inode = new_inode; // inserisco nella tabella
        impl->inode_table[pos_inode].i_node_n = i_node_number; // inserisco numero inode
        impl->inode_table[pos_inode].ref = 0; // azzero il riferimento
    }

    hal_mutex_unlock(&(impl->inode_lock));

    // aggiornamento dei riferimenti
    for( i = 0; i < (impl->dim_inode_table-impl->free_inode); i++)
        impl->inode_table[i].ref >>= 1;

    impl->inode_table[pos_inode].ref |= 0x8000;

    //kprintf ("\n\rinode inserito nella tabella e restituito");
    //kprintf ("\n\rpos_inode %u",pos_inode);
    //kprintf ("\n\rinode number %u",inode_table[pos_inode].i_node_n);
    //stampa_i(&inode_table[pos_inode].inode);
    // restituisco puntatore all'inode cercato
    return &(impl->inode_table[pos_inode].inode);
}


//--------------- Inizializzazione Tabella Inode ---------------//
static bool init_inode_table(e2impl_t *impl)
{
    //int i;
    // la cache degli inode puo` contenere il 1% degli inode su disco
    impl->dim_inode_table = impl->super->s_inodes_count / 100;
    if(!(impl->inode_table = (struct i_node_tab*)malloc(sizeof(struct i_node_tab)*impl->dim_inode_table)))
    {
        SHOW_ERROR0( 0, "Can't create inode table: out of memory");
        return FALSE;
    }

    memset( impl->inode_table, 0, sizeof(struct i_node_tab) * impl->dim_inode_table);

    /*for (i = 0; i < dim_inode_table; i++)
    {
        inode_table[i].i_node_n = 0;
        inode_table[i].ref = 0;
    }*/

    // tutti gli elementi della tabella sono liberi
    impl->free_inode = impl->dim_inode_table;
    //kprintf("\n\rtabella inode creata correttamente");
    return TRUE;
}


// given an inode tells me where and 'block is located in (0 error reading)
static int Inode2Block(e2impl_t *impl, int ino)
{
    struct group_descriptor* group_desc;

    if(!ino || ino > impl->super->s_inodes_count)
    {
        SHOW_FLOW( 2, "invalid inode number %d", ino );
        return 0;
    }

    // il primo inode parte da 1
    ino--;


    // si cerca il descrittore del gruppo di blocchi in cui si trova l'inode
    if(!(group_desc = get_group_desc(impl, ino / impl->super->s_inodes_per_group)))
    {
        SHOW_FLOW( 2, "No group desc for inode %d", ino );
        return 0;
    }

    ino %= impl->super->s_inodes_per_group;

    // bg_inode_table punta al primo blocco tabella inode
    //SHOW_FLOW( 2, "%u",group_desc->bg_inode_table+ino/ipb);
    return (group_desc->bg_inode_table+ino / impl->ipb);
}






// stampa su video il contenuto di un inode

void e2fs_dump_i(struct i_node *ino)
{
    int i;

    SHOW_FLOW( 0, "\n\rtipo inode: %X", ino->i_mode & MODE_MASK);
    SHOW_FLOW( 0, "user id associato al file: %u", ino->i_uid);
    SHOW_FLOW( 0, "file size (byte): %u", ino->i_size);
    SHOW_FLOW( 0, "numero blocchi riservati al file: %u", ino->i_blocks);
    for( i=0; i < 15; i++ )
        SHOW_FLOW( 0, "block[%u]: %u", i, ino->i_block[i] );
    //kgetchar();
}





// -----------------------------------------------------------------------
// Groups
// -----------------------------------------------------------------------



//------------ Inizializzazione Tabella Descrittori di gruppo ------------//
bool init_group_desc_table(e2impl_t *impl)
{
    if(!( impl->group_desc_table = (struct group_descriptor*)malloc(sizeof(struct group_descriptor)*impl->number_of_groups)))
    {
        SHOW_ERROR0( 2, "Can't create group descriptor table: out of memory");
        return FALSE;
    }
    memset( impl->group_desc_table, 0, sizeof(struct group_descriptor)*impl->number_of_groups );

    void * data_block = malloc(impl->dim_block);
    memset(data_block,0,impl->dim_block);

    int err;
    if( (err = impl->ReadSectorsLba( (int64_t)blkno_to_LBA(impl, impl->sbpos), impl->spb, data_block)) )
    {
        // si stampa l'errore relativo al fallimento lettura disco
        SHOW_ERROR0( 2, "error reading group descriptor block" );
        //ShowIdeErrorMessage(err,TRUE);
        return FALSE;
    }

    memcpy( impl->group_desc_table, data_block, sizeof(struct group_descriptor) * impl->number_of_groups);

    free(data_block);

    return TRUE;
}


struct group_descriptor * get_group_desc( e2impl_t *impl, int grp)
{
    if( grp > impl->number_of_groups)
    {
        SHOW_ERROR( 2, "Invalid group descriptor number %d", grp );
        return 0;
    }

    return impl->group_desc_table + grp;
}


void ext2_dump_gd(struct group_descriptor *bg){
    SHOW_FLOW( 2, "id primo blocco bitmap blocchi: %u",bg->bg_block_bitmap);
    SHOW_FLOW( 2, "id primo blocco bitmap inode: %u",bg->bg_inode_bitmap);
    SHOW_FLOW( 2, "id primo blocco tabella inode: %u",bg->bg_inode_table);
    //kgetchar();
}










// -----------------------------------------------------------------------
// FS Init
// -----------------------------------------------------------------------


//--------------- Disk read Ext2 ---------------//
static bool read_ext2(e2impl_t *impl)
{

    int sec; // ??

    SHOW_FLOW0( 1, "Initializing");

    /*
     reading the superblock from START_SUPER_BLOCK. Super block
     is always in the first block of the partition and occupies exactly 1024 bytes
     */

    void *data_block = malloc(DIM_SUPER_BLOCK);
    memset(data_block,0,DIM_SUPER_BLOCK);

    sec = 0;

    int err;

    if ( (err = impl->ReadSectorsLba((int64_t) blkno_to_LBA(impl,0)+sec,2,(word *) data_block)) )
    {
        // si stampa l'errore relativo al fallimento lettura disco
        //ShowIdeErrorMessage(err,TRUE);
        free(data_block);
        return FALSE;
    }
    else
    {
        // si copia il settore nella variabile super
        // *** Andrea Righi ****************************//
        //! super must be allocated first...
        impl->super = malloc(sizeof(struct super_block));
        // *********************************************//
        memcpy(impl->super,data_block,sizeof(struct super_block));
        free(data_block);
        return TRUE;
    }

}


//------------- Controllo validita` Ext2 ---------------//
static bool check_ext2(e2impl_t *impl)
{
    if ((impl->super->s_magic != N_EXT2_NUMERO_MAGICO)&&(impl->super->s_magic != P_EXT2_NUMERO_MAGICO))
    {
        SHOW_ERROR( 1, "Wrong magic %X", impl->super->s_magic );
        return FALSE;
    }
    return TRUE;
}


static bool init_ext2(e2impl_t *impl)
{
    //path_ext2[0] = '\0'; //inizializzazione della path

    hal_mutex_init(&(impl->inode_lock), "Ext2Inode" );
    hal_mutex_init(&(impl->group_lock), "Ext2Group" );

    if (!read_ext2(impl))
    {
        //SHOW_FLOW( 2, "Disk I/O error. Unable to read the super block!!!\n\r");
        return(FALSE);
    }
    // inizializzazione ext2 a buon fine

    if (!check_ext2(impl))
    {
        return(FALSE);
    }
    // ext2 e' valido


    // calcolo parametri della versione corrente file system

    impl->dim_block = 1024 << impl->super->s_log_block_size; //dimensione dei blocchi

    impl->dim_frag = 1024 << impl->super->s_log_frag_size; // dimensione dei frammenti

    impl->spb = impl->dim_block / SIZE_SEC; //settori per blocco

    impl->sbpos = impl->super->s_first_data_block + 1; // posizione del superblocco

    impl->bpg = impl->super->s_blocks_per_group; //blocchi per gruppo

    impl->gdpb = impl->dim_block / sizeof(struct group_descriptor); // desc di gruppo per blocco

    impl->ipb = impl->dim_block / impl->super->s_inode_size; // inodes per blocco dim inode 128 bit

    impl->number_of_groups = impl->super->s_inodes_count / impl->super->s_inodes_per_group; // numero gruppi

    impl->dir_entries_per_block = impl->dim_block / sizeof(struct dir_ff); //directory per blocco

    impl->dim_ptr = impl->dim_block >> 2; // dim del blocco in parole da 32 bit

    // informazioni di carattere generale sulla ext2 corrente
    SHOW_FLOW0( 2, "Ext2 parameters");
    SHOW_FLOW( 2, "Total number of inodes:                         %u", impl->super->s_inodes_count);
    SHOW_FLOW( 2, "Total number of block:                          %u", impl->super->s_blocks_count);
    SHOW_FLOW( 2, "Number of blocks reserved for the super user:   %u", impl->super->s_r_blocks_count);
    SHOW_FLOW( 2, "Number of groups:                               %u", impl->number_of_groups);
    SHOW_FLOW( 2, "Block per group:                                %u", impl->bpg);
    SHOW_FLOW( 2, "Block dimension:                                %u", impl->dim_block);
    SHOW_FLOW( 2, "Fragment dimension:                             %u", impl->dim_frag);
    SHOW_FLOW( 2, "Sector per block:                               %u", impl->spb);
    SHOW_FLOW( 2, "Directories per block:                          %u", impl->dir_entries_per_block);
    SHOW_FLOW( 2, "Group descriptors per block:                    %u", impl->gdpb);
    SHOW_FLOW( 2, "Inodes per group:                               %u", impl->super->s_inodes_per_group);
    SHOW_FLOW( 2, "Frags per group:                                %u", impl->super->s_frags_per_group);
    SHOW_FLOW( 2, "Inodes per block:                               %u", impl->ipb);

    // inizializzazione tabella dei descrittori di gruppo
    if( !init_group_desc_table(impl) )
    {
        return FALSE;
    }

    // inizializzazione tabella degli inode
    if( !init_inode_table(impl) )
    {
        return FALSE;
    }

    // dobbiamo leggere l'inode relativo alla directory radice "inode 2 sempre"
#if 0
    if( !Open_Dir(get_inode(impl,EXT2_ROOT_INO)) )
    {
        // SHOW_FLOW( 2, "Not able to open the root directory\n\r");
        return FALSE;
    }
#endif
    //ino_current_dir = EXT2_ROOT_INO;

    impl->mount_ext2 = TRUE; //file system montato

    //level = 0; // livello nell'albero delle directory, root = 0

    return TRUE;
}





















// -----------------------------------------------------------------------
// Ok, now the real war - read file/dir
// -----------------------------------------------------------------------

// Read first level of indirect blocklist
static bool ReadIndirect1( e2impl_t *impl, int* dst, int* cnt, int blk)
{
    int m;
    // check if there is indirect block at all
    if(*cnt<=0)
        return TRUE;

    //SHOW_FLOW( 2, " read indirect 1");
    int * r1 = malloc(impl->dim_ptr * sizeof(int));
    memset( r1, 0, impl->dim_ptr * sizeof(int));
    // si deve leggere il blocco dati puntato indirettamente

    int err;
    if ( (err = impl->ReadSectorsLba((int64_t)blkno_to_LBA(impl,blk),impl->spb,(word *)r1)) )
    {
        SHOW_ERROR0( 2, "error reading indirect addressing");
        //ShowIdeErrorMessage(err,TRUE);
        free(r1);
        return FALSE;
    }

    /*int*/ m = min( *cnt, impl->dim_ptr );
    // se cnt < dim_ptr vuol dire che non tutti gli indici
    // dei blocchi del blocco indice sono occupati dal file
    memcpy(dst,r1,m<<2);
    free(r1);
    *cnt -= m;
    dst += m;

    return TRUE;
}

bool ReadIndirect2( e2impl_t *impl, int* dst, int* cnt, int blk)
{
    int i;
    if(*cnt <= 0)
    {
        return TRUE;
    }

    //SHOW_FLOW( 2, " read indirect 2");
    int * r2 = malloc(impl->dim_ptr * sizeof(int));

    int err;
    if ( (err = impl->ReadSectorsLba((int64_t)blkno_to_LBA(impl,blk),impl->spb,(word *)r2)) )
    {
        SHOW_ERROR0( 2, "error reading second level indirect addressing");
        //ShowIdeErrorMessage(err,TRUE);
        free(r2);
        return FALSE;
    }

    for (i = 0; *cnt && i < impl->dim_ptr; i++)
        if(!ReadIndirect1(impl,dst,cnt,r2[i]))
        {
            free(r2);
            return FALSE;
        }

    free(r2);
    return TRUE;
}




static bool read_file_blocklist( e2impl_t *impl, struct i_node* ino, int ptr_dir[], int n )
{
    // runs the area of memory 32 bits at a time
    int *ptr = ptr_dir;

    // the first 12 blocks are routed directly
    int m = (n < 12) ? n : 12;

    // memcpy copies bytes therefore do m << 2
    if (m) memcpy(ptr,ino->i_block, m<<2 );

    //SHOW_FLOW( 2, "stampo ptr:%u n:%u m:%u",ptr_dir[0],n,m);
    //kgetchar();
    // blocchi puntati indirettamente
    n-=m;
    // ptr forward to address the statement of indirect blocks
    ptr += m;
    // se ci sono blocchi indiretti si aprono
    if (!ReadIndirect1(impl, ptr, &n, ino->i_block[12]))
    {
        //SHOW_FLOW( 2, "ind1");
        return FALSE;
    }
    // indirizzamento indiretto doppio
    if (!ReadIndirect2(impl, ptr, &n, ino->i_block[13]))
    {
        //SHOW_FLOW( 2, "ind2");
        return FALSE;
    }
    // indirizzamento indiretto triplo
    if(n)
    {
        //SHOW_FLOW( 2, "ind3");
        dword* r3 = (dword *)malloc(impl->dim_ptr * sizeof(dword));
        int err;
        // TODO : lettura indirizzamento triplo
        if( (err = impl->ReadSectorsLba(blkno_to_LBA(impl,ino->i_block[14]),impl->spb,(word *)r3)) )
        {
            SHOW_ERROR0( 2, "error reading third level indirect addressing" );
            //ShowIdeErrorMessage(err,TRUE);
            free(r3);
            return FALSE;
        }

        int i;
        // TODO: recheck the definition of the index
        for(i = 0; n && i < impl->dim_ptr; i++)
            if(!ReadIndirect2(impl,ptr,&n,r3[i]))
            {
                free(r3);
                return FALSE;
            }
        free(r3);
    }

    return TRUE;
    // a questo punto ptr_dir contiene il valore dei puntatori ai blocchi
    // del file aperto
}


// opening a file whether directory, or file link

static bool Open_File( e2impl_t *impl, struct i_node* ino, word tipo_file )
{
    if (ino == 0)
    {
        // si e' verificato un errore nessun inode specificato
        SHOW_ERROR0( 2, "Invalid inode");
        return FALSE;
    }

    // check di validita` dell'inode
    if (!(tipo_file == MODE_DIR || tipo_file == MODE_FILE || tipo_file == MODE_LINK))
    {
        // tipo non valido
        SHOW_ERROR0( 2, "Invalid file type");
        return FALSE;
    }

    // TODO : fast symbolic link
    if ((tipo_file==MODE_LINK) && (ino->i_size<64))
    {
        SHOW_FLOW0( 2, "Fast symbolic link path: ");

        // TODO unicode?
        unsigned char * fsl;

        fsl = (byte *)malloc(sizeof(struct i_node));
        memset(fsl,0,sizeof(struct i_node));
        memcpy(fsl,ino,sizeof(struct i_node));

        int i;
        for (i=39;i<(ino->i_size+39);i++)
            printf("%c",fsl[i]);
        printf("\n\n\r");

        free(fsl);
        return FALSE;
    }

    // read all pointers to data blocks of the file

    // number of blocks that make up the file
    int n = (ino->i_size+impl->dim_block-1) / impl->dim_block;

    // We linearized all pointers to data blocks in order
    // Such as to simplify and optimize the hierarchical structure
    // Blocks access to actual data
    int *ptr_dir = malloc(n * sizeof(int));
    memset(ptr_dir,0,n * sizeof(dword));

    if( !read_file_blocklist( impl, ino, ptr_dir, n ) )
    {
        free(ptr_dir);
        return FALSE;
    }

    return TRUE;
}


#endif // HAVE_UNIX

