/*
type FILE_MODE_TYPE is (READ, READ_WRITE);
type MESSAGE_ADDR_TYPE is a continuous area of data defined
by a starting address (see ARINC 653 Part 1);

type MESSAGE_SIZE_TYPE is a numeric type; -- number of bytes
type FILE_ERRNO_TYPE is numeric type;
type FILE_NAME_TYPE is a n-character string;
type FILE_ID_TYPE is numeric type;
type DIRECTORY_ID_TYPE is numeric type;
type FILE_SIZE_TYPE is numeric type; -- implementation dependent
type FILE_SEEK_TYPE is (SEEK_SET, SEEK_CUR, SEEK_END);
type ENTRY_KIND_TYPE is (FILE_ENTRY,
DIRECTORY_ENTRY,
OTHER_ENTRY,
END_OF_DIRECTORY);
type TIME_SET_TYPE is (UNSET, SET);
type MEDIA_TYPE is (VOLATILE, NONVOLATILE, REMOTE);
type COMPOSITE_TIME_TYPE is record
TM_SEC is numeric type; -- seconds after the minute [0,59]
TM_MIN is numeric type; -- minutes after the hour [0,59]
TM_HOUR is numeric type; -- hours since midnight [0,23]
TM_MDAY is numeric type; -- day of the month [1,31]
TM_MON is numeric type; -- months since January [0,11]
TM_YEAR is numeric type; -- years since 1900
TM_WDAY is numeric type; -- days since Sunday [0,6]
TM_YDAY is numeric type; -- days since January 1 [0,365]
TM_ISDST is numeric type; -- Daylight Savings Time flag
TM_IS_SET : TIME_SET_TYPE; -- time has been set indication
end record;
type FILE_STATUS_TYPE is record
CREATION_TIME : COMPOSITE_TIME_TYPE;
LAST_UPDATE : COMPOSITE_TIME_TIME;
POSITION : FILE_SIZE_TYPE; -- local to a file id
SIZE : FILE_SIZE_TYPE; -- visible to all file ids
NB_OF_CHANGES is numeric type; -- visible to all file ids
NB_OF_WRITE_ERRORS is numeric type; -- visible to all file ids
end record;
type VOLUME_STATUS_TYPE is record
TOTAL_BYTES is numeric type; -- nb of bytes allocated to
-- a volume
USED_BYTES is numeric type; -- nb of bytes used
FREE_BYTES is numeric type; -- nb of bytes available
MAX_ATOMIC_SIZE is numeric type; -- nb of bytes that can be
-- atomically transferred
BLOCK_SIZE is numeric type; -- nb of consecutive bytes
-- per block
ACCESS_RIGHTS : FILE_MODE_TYPE; -- configuration defined access rights
MEDIA : MEDIA_TYPE; -- configuration defined media type
end record;

*/


/*
Errno list

EACCES Permission denied
EBADF Bad file descriptor (e.g., does not refer to an open file or directory)
EBUSY Resource busy
EEXIST File exists
EFBIG Read or write size is greater than maximum atomicity
EINVAL Invalid argument
EIO Input/output error
EISDIR Is a directory
EMFILE Too many open files (by the current partition)
ENAMETOOLONG Filename too long
ENOENT No such file or directory (e.g., a component of path prefix does not name
an existing file or the name is an empty string)
ENOSPC No space left on volume
ENOTDIR Not a volume or directory (e.g., a component of the path prefix is a file)
ENOTEMPTY Directory not empty
EOVERFLOW Current file position beyond end of file
EPERM Operation not permitted
EROFS Storage device that would contain a file is currently write protected
ESTALE File or directory ID which an application has been using is no longer
valid


*/


/*

OPEN_NEW_FILE
OPEN_FILE
CLOSE_FILE
READ_FILE
WRITE_FILE
SEEK_FILE
REMOVE_FILE
RENAME_FILE
GET_FILE_STATUS
GET_VOLUME_STATUS
RESIZE_FILE
SYNC_FILE
OPEN_DIRECTORY
CLOSE_DIRECTORY
READ_DIRECTORY
REWIND_DIRECTORY
MAKE_DIRECTORY
REMOVE_DIRECTORY
SYNC_DIRECTORY
RENAME_DIRECTORY

*/


/*

procedure OPEN_NEW_FILE
(FILE_NAME : in FILE_NAME_TYPE;
FILE_ID : out FILE_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure OPEN_FILE
(FILE_NAME : in FILE_NAME_TYPE;
FILE_MODE : in FILE_MODE_TYPE;
FILE_ID : out FILE_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure CLOSE_FILE
(FILE_ID : in FILE_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure READ_FILE
(FILE_ID : in FILE_ID_TYPE;
MESSAGE_ADDR : in MESSAGE_ADDR_TYPE;
-- the message address is passed IN, although the respective
-- message is passed OUT
IN_LENGTH : in MESSAGE_SIZE_TYPE; -- max number of bytes to read
OUT_LENGTH : out MESSAGE_SIZE_TYPE; -- number of bytes actually read
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure WRITE_FILE
(FILE_ID : in FILE_ID_TYPE;
MESSAGE_ADDR : in MESSAGE_ADDR_TYPE;
LENGTH : in MESSAGE_SIZE_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure SEEK_FILE
(FILE_ID : in FILE_ID_TYPE;
OFFSET : in FILE_SIZE_TYPE;
WHENCE : in FILE_SEEK_TYPE;
POSITION : out FILE_SIZE_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure REMOVE_FILE
(FILE_NAME : in FILE_NAME_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure RENAME_FILE
(OLD_FILE_NAME : in FILE_NAME_TYPE;
NEW_FILE_NAME : in FILE_NAME_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure GET_FILE_STATUS
(FILE_ID : in FILE_ID_TYPE;
FILE_STATUS : out FILE_STATUS_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure GET_VOLUME_STATUS
(FILE_NAME : in FILE_NAME_TYPE;
VOLUME_STATUS : out VOLUME_STATUS_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure RESIZE_FILE
(FILE_ID : in FILE_ID_TYPE;
NEW_SIZE : in FILE_SIZE_TYPE,
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure SYNC_FILE
(FILE_ID : in FILE_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure OPEN_DIRECTORY
(DIRECTORY_NAME : in FILE_NAME_TYPE;
DIRECTORY_ID : out DIRECTORY_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure CLOSE_DIRECTORY
(DIRECTORY_ID : in DIRECTORY_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)


procedure READ_DIRECTORY
(DIRECTORY_ID : in DIRECTORY_ID_TYPE;
ENTRY_NAME : out DIRECTORY_ENTRY_TYPE;
ENTRY_KIND : out ENTRY_KIND_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)



procedure REWIND_DIRECTORY
(DIRECTORY_ID : in DIRECTORY_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)



procedure MAKE_DIRECTORY
(DIRECTORY_NAME : in FILE_NAME_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure REMOVE_DIRECTORY
(DIRECTORY_NAME : in FILE_NAME_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure SYNC_DIRECTORY
(DIRECTORY_ID : in DIRECTORY_ID_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)

procedure RENAME_DIRECTORY
(OLD_DIRECTORY_NAME : in FILE_NAME_TYPE;
NEW_DIRECTORY_NAME : in FILE_NAME_TYPE;
RETURN_CODE : out RETURN_CODE_TYPE;
ERRNO : in out FILE_ERRNO_TYPE)






*/



