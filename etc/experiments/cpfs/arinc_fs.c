/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * ARINC-653 File System entry points.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"
#include "arinc_fs.h"


static void fill_arinc_retcodes( errno_t rc, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );
static errno_t arinc_find_filesystem( const char *full_file_name, struct cpfs_fs **fs, size_t *name_pos );
static void *arinc_get_user_data( void ) { return 0; }
static void arinc_time( COMPOSITE_TIME_TYPE *out_time, cpfs_time_t in_time );

// ----------------------------------------------------------------------------
//
// Open/close functions
//
// ----------------------------------------------------------------------------



void OPEN_NEW_FILE( FILE_NAME_TYPE FILE_NAME, FILE_ID_TYPE *FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( FILE_NAME, &fs, &np );
    if( rc ) goto fail;


    // TODO O_EXCL? Can't open existing?

    rc = cpfs_file_open( fs, FILE_ID, FILE_NAME+np, O_CREAT, arinc_get_user_data() );

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void OPEN_FILE( FILE_NAME_TYPE FILE_NAME, FILE_MODE_TYPE FILE_MODE, FILE_ID_TYPE *FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( FILE_NAME, &fs, &np );
    if( rc ) goto fail;

    // TODO FILE_MODE

    rc = cpfs_file_open( fs, FILE_ID, FILE_NAME+np, 0, arinc_get_user_data() );

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void CLOSE_FILE( FILE_ID_TYPE FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    rc = cpfs_file_close( FILE_ID );

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}




// ----------------------------------------------------------------------------
//
// Main file functions
//
// ----------------------------------------------------------------------------



void READ_FILE( FILE_ID_TYPE FILE_ID, MESSAGE_ADDR_TYPE MESSAGE_ADDR, MESSAGE_SIZE_TYPE IN_LENGTH, MESSAGE_SIZE_TYPE *OUT_LENGTH, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    cpfs_size_t pos = 0; // TODO pos

    rc = cpfs_file_read( FILE_ID, pos, MESSAGE_ADDR, IN_LENGTH );

    if( rc == 0 )
        *OUT_LENGTH = IN_LENGTH;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void WRITE_FILE( FILE_ID_TYPE FILE_ID, MESSAGE_ADDR_TYPE MESSAGE_ADDR, MESSAGE_SIZE_TYPE LENGTH, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    cpfs_size_t pos = 0; // TODO pos

    rc = cpfs_file_write( FILE_ID, pos, MESSAGE_ADDR, LENGTH );

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}


/*
void SEEK_FILE( FILE_ID_TYPE FILE_ID, FILE_SIZE_TYPE OFFSET, FILE_SEEK_TYPE WHENCE, FILE_SIZE_TYPE *POSITION, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}
*/


// ----------------------------------------------------------------------------
//
// File name functions
//
// ----------------------------------------------------------------------------



void REMOVE_FILE( FILE_NAME_TYPE FILE_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( FILE_NAME, &fs, &np );
    if( rc ) goto fail;


    // TODO is not dir

    rc = cpfs_file_unlink( fs, FILE_NAME+np, arinc_get_user_data() );

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}

void RENAME_FILE( FILE_NAME_TYPE OLD_FILE_NAME, FILE_NAME_TYPE NEW_FILE_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



// ----------------------------------------------------------------------------
//
// Status functions
//
// ----------------------------------------------------------------------------



void GET_FILE_STATUS( FILE_ID_TYPE FILE_ID, FILE_STATUS_TYPE *FILE_STATUS, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    struct cpfs_stat stat;

    rc = cpfs_fd_stat( FILE_ID, &stat );

#warning impl time
#warning impl pos
#warning impl n changes/errors

    //FILE_STATUS->CREATION_TIME = ;
    //FILE_STATUS->LAST_UPDATE = ;
    FILE_STATUS->POSITION = 0;
    FILE_STATUS->SIZE = stat.fsize;
    FILE_STATUS->NB_OF_CHANGES = 0;
    FILE_STATUS->NB_OF_WRITE_ERRORS = 0;

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}

void GET_VOLUME_STATUS( FILE_NAME_TYPE FILE_NAME, VOLUME_STATUS_TYPE *VOLUME_STATUS, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( FILE_NAME, &fs, &np );
    if( rc ) goto fail;

    cpfs_blkno_t disk_size;
    cpfs_blkno_t disk_free;

    rc = cpfs_fs_stat( fs, &disk_size, &disk_free );

    VOLUME_STATUS->TOTAL_BYTES = disk_size;
    VOLUME_STATUS->USED_BYTES = disk_size - disk_free;
    VOLUME_STATUS->FREE_BYTES = disk_free;
    VOLUME_STATUS->MAX_ATOMIC_SIZE = 0; // TODO ?
    VOLUME_STATUS->BLOCK_SIZE = CPFS_BLKSIZE;
    VOLUME_STATUS->ACCESS_RIGHTS = WRITE; // TODO hardcode
    VOLUME_STATUS->MEDIA = NONVOLATILE; // TODO hardcode

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



// ----------------------------------------------------------------------------
//
// File size/sync functions
//
// ----------------------------------------------------------------------------


/*
void RESIZE_FILE( FILE_ID_TYPE FILE_ID, FILE_SIZE_TYPE NEW_SIZE, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}

void SYNC_FILE( FILE_ID_TYPE FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}
*/


// ----------------------------------------------------------------------------
//
// Read directory functions
//
// ----------------------------------------------------------------------------

/*

void OPEN_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, DIRECTORY_ID_TYPE *DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}

void CLOSE_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}

//void READ_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, DIRECTORY_ENTRY_TYPE *ENTRY_NAME, ENTRY_KIND_TYPE *ENTRY_KIND, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )

void REWIND_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}
*/


// ----------------------------------------------------------------------------
//
// Directory functions
//
// ----------------------------------------------------------------------------



void MAKE_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( DIRECTORY_NAME, &fs, &np );
    if( rc ) goto fail;

    rc = cpfs_mkdir( fs, DIRECTORY_NAME+np, arinc_get_user_data() );

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void REMOVE_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;
    struct cpfs_fs *fs;
    size_t np;

    rc = arinc_find_filesystem( DIRECTORY_NAME, &fs, &np );
    if( rc ) goto fail;

    // TODO isdir

    rc = cpfs_file_unlink( fs, DIRECTORY_NAME+np, arinc_get_user_data() );

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void SYNC_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

#warning impl me

    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}



void RENAME_DIRECTORY( FILE_NAME_TYPE OLD_DIRECTORY_NAME, FILE_NAME_TYPE NEW_DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    errno_t rc = 0;

    struct cpfs_fs *fs1;
    struct cpfs_fs *fs2;

    size_t np1, np2;

    rc = arinc_find_filesystem( OLD_DIRECTORY_NAME, &fs1, &np1 );
    if( rc ) goto fail;

    rc = arinc_find_filesystem( NEW_DIRECTORY_NAME, &fs2, &np2 );
    if( rc ) goto fail;

    if( fs1 != fs2 )
    {
        cpfs_log_error( "Attempt to rename dir to differrent volume ('%s' to '%s')", OLD_DIRECTORY_NAME, NEW_DIRECTORY_NAME );
        rc = EINVAL;
        goto fail;
    }

    // TODO check that fn1 is dir
    // TODO IMPL me
#warning impl me

    cpfs_log_error( "Renema dir not impl" );
    rc = EIO;

fail:
    fill_arinc_retcodes( rc, RETURN_CODE, ERRNO );
}






// ----------------------------------------------------------------------------
//
// Arinc utils
//
// ----------------------------------------------------------------------------


// Basic mapping of errno to retcode, CHECK IF IT IS OK FOR EACH FUNC!
static void fill_arinc_retcodes( errno_t rc, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO )
{
    *ERRNO = rc;

    switch(rc)
    {
    case EMFILE:	*RETURN_CODE = INVALID_CONFIG;

    case EIO:    	*RETURN_CODE = NOT_AVAILABLE;

    case EACCES:    	*RETURN_CODE = INVALID_MODE;

    default:    	*RETURN_CODE = INVALID_PARAM;
    };

}



// ----------------------------------------------------------------------------
//
// Volumes support
//
// ----------------------------------------------------------------------------


typedef struct
{
    const char          *name;          // Volume name

    int                 disk_id;        // Disk number (used in driver calls)

    cpfs_blkno_t        disk_size;      // Disk size, 4K blocks

    struct cpfs_fs 	fs;            // FS instance (TODO ptr?)

    int                 active;
}
arinc_volume_def_t;

arinc_volume_def_t volumes[] =
{
    { .name = "vol0", .disk_id = 0, .disk_size = 10000 },
    { .name = "vol1", .disk_id = 1, .disk_size = 12000 }
};

static int nvol = sizeof(volumes) / sizeof(arinc_volume_def_t);

void
arinc_init_filesystems( void )
{
    int i;
    errno_t rc;

    for( i = 0; i < nvol; i++ )
    {
        volumes[i].fs.disk_id = volumes[i].disk_id;
        volumes[i].fs.disk_size = volumes[i].disk_size;

        rc = cpfs_init( &volumes[i].fs );
        if( rc )
        {
            cpfs_log_error( "Can't init FS %d (vol '%s'), rc = %d", i, volumes[i].name, rc );
            continue;
        }

        rc = cpfs_mount( &volumes[i].fs );
        if( rc )
        {
            cpfs_log_error( "Can't mount FS %d (vol '%s'), rc = %d", i, volumes[i].name, rc );
            continue;
        }

        volumes[i].active = 1;
    }
}

errno_t
arinc_find_filesystem( const char *full_file_name, struct cpfs_fs **fs, size_t *name_pos )
{
    int i;

    for( i = 0; i < nvol; i++ )
    {
        if( !volumes[i].active )
            continue;

        size_t len = strlen( volumes[i].name );

        if( (full_file_name[len] != ':') && (full_file_name[len] != '/') )
            continue;

        if( strncmp( volumes[i].name, full_file_name, len ) )
            continue;

        *name_pos = len+1;
        *fs = &(volumes[i].fs);

        return 0;
    }

    return ENOENT;
}



