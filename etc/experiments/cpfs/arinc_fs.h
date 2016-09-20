/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * ARINC-653 File System types and defs.
 *
 *
**/

#include "cpfs_types.h"

// TODO right? order? numeric value?

typedef enum
{
    INVALID_CONFIG,
    INVALID_PARAM,
    INVALID_MODE,
    NOT_AVAILABLE,
}
RETURN_CODE_TYPE;



// type FILE_MODE_TYPE is (READ, READ_WRITE);

typedef enum { READ, WRITE } FILE_MODE_TYPE;

// type MESSAGE_ADDR_TYPE is a continuous area of data defined by a starting address (see ARINC 653 Part 1);

typedef void* MESSAGE_ADDR_TYPE;

// type MESSAGE_SIZE_TYPE is a numeric type; -- number of bytes

typedef size_t MESSAGE_SIZE_TYPE;

// type FILE_SEEK_TYPE is (SEEK_SET, SEEK_CUR, SEEK_END);

typedef enum { ARINC_SEEK_SET, ARINC_SEEK_CUR, ARINC_SEEK_END } FILE_SEEK_TYPE;

// type FILE_ERRNO_TYPE is numeric type;

typedef errno_t FILE_ERRNO_TYPE;

// type FILE_ID_TYPE is numeric type;

typedef int FILE_ID_TYPE;

// type DIRECTORY_ID_TYPE is numeric type;

typedef int DIRECTORY_ID_TYPE;


// type ENTRY_KIND_TYPE is (FILE_ENTRY, DIRECTORY_ENTRY, OTHER_ENTRY, END_OF_DIRECTORY);

typedef enum { FILE_ENTRY, DIRECTORY_ENTRY, OTHER_ENTRY, END_OF_DIRECTORY } ENTRY_KIND_TYPE;

// type FILE_NAME_TYPE is a n-character string;

typedef const char * FILE_NAME_TYPE;

// type FILE_SIZE_TYPE is numeric type; -- implementation dependent

typedef cpfs_fpos_t FILE_SIZE_TYPE;

// type TIME_SET_TYPE is (UNSET, SET);

typedef enum { UNSET, SET } TIME_SET_TYPE;

// type MEDIA_TYPE is (VOLATILE, NONVOLATILE, REMOTE);

typedef enum { VOLATILE, NONVOLATILE, REMOTE } MEDIA_TYPE;

// type COMPOSITE_TIME_TYPE is record
// 	TM_SEC is numeric type; -- seconds after the minute [0,59]
// 	TM_MIN is numeric type; -- minutes after the hour [0,59]
// 	TM_HOUR is numeric type; -- hours since midnight [0,23]
// 	TM_MDAY is numeric type; -- day of the month [1,31]
// 	TM_MON is numeric type; -- months since January [0,11]
// 	TM_YEAR is numeric type; -- years since 1900
// 	TM_WDAY is numeric type; -- days since Sunday [0,6]
// 	TM_YDAY is numeric type; -- days since January 1 [0,365]
// 	TM_ISDST is numeric type; -- Daylight Savings Time flag
// 	TM_IS_SET : TIME_SET_TYPE; -- time has been set indication
// end record;

typedef struct
{
	int		TM_SEC;		// seconds after the minute [0,59]
	int 		TM_MIN;		// minutes after the hour [0,59]
	int 		TM_HOUR;	// hours since midnight [0,23]
	int 		TM_MDAY;	// day of the month [1,31]
	int 		TM_MON;		// months since January [0,11]
	int 		TM_YEAR;	// years since 1900
	int 		TM_WDAY;	// days since Sunday [0,6]
	int 		TM_YDAY;	// days since January 1 [0,365]
        int 		TM_ISDST;	// Daylight Savings Time flag
	TIME_SET_TYPE 	TM_IS_SET; 	// time has been set indication
}
COMPOSITE_TIME_TYPE;

// type FILE_STATUS_TYPE is record
//	CREATION_TIME : COMPOSITE_TIME_TYPE;
//	LAST_UPDATE : COMPOSITE_TIME_TIME;
//	POSITION : FILE_SIZE_TYPE; -- local to a file id
//	SIZE : FILE_SIZE_TYPE; -- visible to all file ids
//	NB_OF_CHANGES is numeric type; -- visible to all file ids
//	NB_OF_WRITE_ERRORS is numeric type; -- visible to all file ids
// end record;


typedef struct
{
	COMPOSITE_TIME_TYPE		CREATION_TIME;
	COMPOSITE_TIME_TYPE		LAST_UPDATE;
	FILE_SIZE_TYPE			POSITION; 		// local to a file id
	FILE_SIZE_TYPE			SIZE; 			// visible to all file ids
	int				NB_OF_CHANGES; 		// visible to all file ids
	int				NB_OF_WRITE_ERRORS;	// visible to all file ids
}
FILE_STATUS_TYPE;




// type VOLUME_STATUS_TYPE is record
//	TOTAL_BYTES is numeric type; -- nb of bytes allocated to a volume
//	USED_BYTES is numeric type; -- nb of bytes used
//	FREE_BYTES is numeric type; -- nb of bytes available
//	MAX_ATOMIC_SIZE is numeric type; -- nb of bytes that can be atomically transferred
//	BLOCK_SIZE is numeric type; -- nb of consecutive bytes per block
//	ACCESS_RIGHTS : FILE_MODE_TYPE; -- configuration defined access rights
//	MEDIA : MEDIA_TYPE; -- configuration defined media type
// end record;



typedef struct
{
    	cpfs_blkno_t	TOTAL_BYTES;		// nb of bytes allocated to a volume
    	cpfs_blkno_t	USED_BYTES;		// nb of bytes used
    	cpfs_blkno_t	FREE_BYTES;		// nb of bytes available
    	cpfs_blkno_t	MAX_ATOMIC_SIZE;	// nb of bytes that can be atomically transferred
	int		BLOCK_SIZE;		// nb of consecutive bytes per block
	FILE_MODE_TYPE	ACCESS_RIGHTS;		// configuration defined access rights
        MEDIA_TYPE	MEDIA;			// configuration defined media type
}
VOLUME_STATUS_TYPE;






/**
 *
 * Errno list
 *
 *	EACCES 		Permission denied
 *	EBADF 		Bad file descriptor (e.g., does not refer to an open file or directory)
 *	EBUSY 		Resource busy
 *	EEXIST 		File exists
 *	EFBIG 		Read or write size is greater than maximum atomicity
 *	EINVAL 		Invalid argument
 *	EIO 		Input/output error
 *	EISDIR 		Is a directory
 *	EMFILE 		Too many open files (by the current partition)
 *	ENAMETOOLONG	Filename too long
 *	ENOENT 		No such file or directory (e.g., a component of path prefix does not name
 *			an existing file or the name is an empty string)
 *	ENOSPC 		No space left on volume
 *	ENOTDIR 	Not a volume or directory (e.g., a component of the path prefix is a file)
 *	ENOTEMPTY 	Directory not empty
 *	EOVERFLOW 	Current file position beyond end of file
 *	EPERM 		Operation not permitted
 *	EROFS 		Storage device that would contain a file is currently write protected
 *	ESTALE 		File or directory ID which an application has been using is no longer valid
 *
**/




void OPEN_NEW_FILE( FILE_NAME_TYPE FILE_NAME, FILE_ID_TYPE *FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );

void OPEN_FILE( FILE_NAME_TYPE FILE_NAME, FILE_MODE_TYPE FILE_MODE, FILE_ID_TYPE *FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );

void CLOSE_FILE( FILE_ID_TYPE FILE_ID, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );



void READ_FILE( FILE_ID_TYPE FILE_ID, MESSAGE_ADDR_TYPE MESSAGE_ADDR, MESSAGE_SIZE_TYPE IN_LENGTH, MESSAGE_SIZE_TYPE *OUT_LENGTH, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );

void WRITE_FILE( FILE_ID_TYPE FILE_ID, MESSAGE_ADDR_TYPE MESSAGE_ADDR, MESSAGE_SIZE_TYPE LENGTH, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );

void SEEK_FILE( FILE_ID_TYPE FILE_ID, FILE_SIZE_TYPE OFFSET, FILE_SEEK_TYPE WHENCE, FILE_SIZE_TYPE *POSITION, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );



void REMOVE_FILE( FILE_NAME_TYPE FILE_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );

void RENAME_FILE( FILE_NAME_TYPE OLD_FILE_NAME, FILE_NAME_TYPE NEW_FILE_NAME, RETURN_CODE_TYPE *RETURN_CODE, FILE_ERRNO_TYPE *ERRNO );



void GET_FILE_STATUS( FILE_ID_TYPE FILE_ID, FILE_STATUS_TYPE *FILE_STATUS, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void GET_VOLUME_STATUS( FILE_NAME_TYPE FILE_NAME, VOLUME_STATUS_TYPE *VOLUME_STATUS, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );



void RESIZE_FILE( FILE_ID_TYPE FILE_ID, FILE_SIZE_TYPE NEW_SIZE, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void SYNC_FILE( FILE_ID_TYPE FILE_ID, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );



void OPEN_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, DIRECTORY_ID_TYPE *DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void CLOSE_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

//void READ_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, DIRECTORY_ENTRY_TYPE *ENTRY_NAME, ENTRY_KIND_TYPE *ENTRY_KIND, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void REWIND_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );



void MAKE_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void REMOVE_DIRECTORY( FILE_NAME_TYPE DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void SYNC_DIRECTORY( DIRECTORY_ID_TYPE DIRECTORY_ID, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );

void RENAME_DIRECTORY( FILE_NAME_TYPE OLD_DIRECTORY_NAME, FILE_NAME_TYPE NEW_DIRECTORY_NAME, RETURN_CODE_TYPE *RETURN_CODE_TYPE, FILE_ERRNO_TYPE *ERRNO );










