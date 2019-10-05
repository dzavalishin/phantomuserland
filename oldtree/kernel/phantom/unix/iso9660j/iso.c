// iso.c
typedef long long dr9_off_t;

#include <malloc.h>
#include <SupportDefs.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Drivers.h>
#include <OS.h>
#include <lock.h>
#include <cache.h>
#include <KernelExport.h> 
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <malloc.h>
//#include "dmalloc.h"

#include "rock.h"
#include "iso.h"


// Size of primary volume descriptor for ISO9660
#define ISO_PVD_SIZE 882

// ISO9660 should start with this string
const char* 	kISO9660IDString = "CD001";

static int   	GetLogicalBlockSize(int fd);
static int   	GetDeviceBlockSize(int fd);
static off_t 	GetNumDeviceBlocks(int fd, int block_size);

static int		InitVolDate(ISOVolDate* date, char* buf);
static int		InitRecDate(ISORecDate* date, char* buf);
static int		InitVolDesc(nspace* vol, char* buf);

static int
GetLogicalBlockSize(int fd)
{
    partition_info  p_info;

    if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR)
    {
    	//dprintf("GetLogicalBlockSize: ioctl suceed\n");
		return p_info.logical_block_size;
	}
    else //dprintf("GetLogicalBlockSize = ioctl returned error\n");
	return 0;
}

static int
GetDeviceBlockSize(int fd)
{
    struct stat     st;
    device_geometry dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg) < 0) 
    {
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
	    	return 0;
		return 512;   /* just assume it's a plain old file or something */
    }
    
    return dg.bytes_per_sector;
}

static off_t
GetNumDeviceBlocks(int fd, int block_size)
{
    struct stat			st;
    device_geometry		dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg) >= 0) 
    {
		return (off_t)dg.cylinder_count *
	       (off_t)dg.sectors_per_track *
	       (off_t)dg.head_count;
    }

    /* if the ioctl fails, try just stat'ing in case it's a regular file */
    if (fstat(fd, &st) < 0)
	return 0;

    return st.st_size / block_size;
}

// Functions specific to iso driver.
int
ISOMount(const char *path, const int flags, nspace** newVol)
{
	// path: 		path to device (eg, /dev/disk/scsi/030/raw)
	// partition:	partition number on device ????
	// flags:		currently unused
	
	#pragma unused (flags)
	nspace*		vol = NULL;
	int 		result = B_NO_ERROR;
	
	//dprintf("ISOMount - ENTER\n");
	
	vol = (nspace*)calloc(sizeof(nspace), 1);
	if (vol == NULL)
	{
		/// error.
		result = ENOMEM;
		//dprintf("ISOMount - mem error \n");
	}
	else
	{		
		/* open and lock the device */
		vol->fd = open(path, O_RDONLY);
		if (vol->fd >= 0)
		{
			//int deviceBlockSize, logicalBlockSize, multiplier;
			int deviceBlockSize, multiplier;
			
			deviceBlockSize =  GetDeviceBlockSize(vol->fd);
			if (deviceBlockSize == 0) 
			{
				// Error
				//dprintf("ISO9660 ERROR - device block size is 0\n");
				if (vol->fd >= 0) close(vol->fd);
				result = EINVAL;
				free(vol);
			}
			else
			{
				// determine if it is an ISO volume.
				char buf[ISO_PVD_SIZE];
				bool done = false, is_iso = false;
				off_t offset = 0x8000;

				// change by GJvR:
				// instead of just reading the first volume descriptor,
				// we'll try the first 16 sectors to see if they contain something useful
				// until we hit the ISO_VD_END block.
				vol->joliet_level = 0;
				while((!done) && (offset < 0x10000))
				{
					read_pos (vol->fd, offset, (void*)buf, ISO_PVD_SIZE);
					//dprintf("data at %x: %s\n", offset, (char*)buf);
					if (!strncmp(buf+1, kISO9660IDString, 5))
					{
						if ((*buf == 0x01) && (!is_iso)) // ISO_VD_PRIMARY
						{
							//dprintf("ISOMount: Is an ISO9660 volume, initting rec\n");
							InitVolDesc(vol, buf);
							strncpy(vol->devicePath,path,127);
							vol->id = ISO_ROOTNODE_ID;
							//dprintf("ISO9660: vol->blockSize = %ld\n",vol->logicalBlkSize[FS_DATA_FORMAT]); 
							multiplier = deviceBlockSize / vol->logicalBlkSize[FS_DATA_FORMAT];
							//dprintf("ISOMount: block size multiplier is %ld\n", multiplier);
			  		
							/* Initialize access to the cache so that we can do cached i/o */
							//dprintf("ISO9660: cache init: dev %d, max blocks %Ld\n", vol->fd, vol->volSpaceSize[FS_DATA_FORMAT]);
							init_cache_for_device(vol->fd, vol->volSpaceSize[FS_DATA_FORMAT]);
							is_iso = true;
						}
						else if ((*buf == 0x02) && (is_iso)) // ISO_VD_SUPPLEMENTARY
						{
							// JOLIET extension  GJvR
							// test escape sequence for level of UCS-2 characterset
						    if (buf[88] == 0x25 && buf[89] == 0x2f)
						    {
								switch(buf[90])
						    	{
									case 0x40: vol->joliet_level = 1; break;
									case 0x43: vol->joliet_level = 2; break;
									case 0x45: vol->joliet_level = 3; break;
								}
							
								//dprintf("ISO9660 Extensions: Microsoft Joliet Level %d\n", vol->joliet_level);

								// Because Joliet-stuff starts at other sector,
								// update root directory record.
								if (vol->joliet_level > 0)
									InitNode(&(vol->rootDirRec), &buf[156], NULL, 0);
							}
						}
						else if (*(unsigned char *)buf == 0xff) // ISO_VD_END
						{
							done = true;
						}
					} // end if (!strncmp(buf+1, kISO9660IDString, 5))
					offset += 0x800;
				} // end while(!done)

				if (!is_iso)
				{
					// It isn't an ISO disk.
					if (vol->fd >= 0) close(vol->fd);
					free(vol);
					vol = NULL;
					result = EINVAL;
					//dprintf("ISOMount: Not an ISO9660 volume!\n");
				}
				else
				{
					// show joliet-level in CD title
					// (better to remove this in final version :)
					int i;
					i = strlen(vol->volIDString);
					if (i > 29) i = 29;
					sprintf(&vol->volIDString[i], " [%d]", vol->joliet_level);
				}
			}
		}
		else
		{
			//dprintf("ISO9660 ERROR - Unable to open <%s>\n", path);
			free(vol);
			vol = NULL;
			result = EINVAL;
		}
	}
	//dprintf("ISOMount - EXIT, result %s, returning 0x%x\n", strerror(result), vol);
	*newVol = vol;
	return result;
}

// ISOReadDirEnt
/* Reads in a single directory entry and fills in the values in the
	dirent struct. Uses the cookie to keep track of the current block
	and position withing the block. Also uses the cookie to determine when
	it has reached the end of the directory file.
	
	NOTE: If your file sytem seems to work ok from the command line, but
	the tracker doesn't seem to like it, check what you do here closely;
	in particular, if the d_ino in the stat struct isn't correct, the tracker
	will not display the entry.
*/
int
ISOReadDirEnt(nspace* ns, dircookie* cookie, struct dirent* buf, size_t bufsize)
{
	dr9_off_t		totalRead = cookie->pos + 
								((cookie->block - cookie->startBlock) *
								ns->logicalBlkSize[FS_DATA_FORMAT]);
	dr9_off_t		cacheBlock;
	char*			blockData;
	int				result = B_NO_ERROR;
	int 			bytesRead = 0;

	//dprintf("ISOReadDirEnt - ENTER\n");
	
	
	// If we're at the end of the data in a block, move to the next block.	
	while (1)
	{
		blockData = (char*)get_block(ns->fd, cookie->block,
							ns->logicalBlkSize[FS_DATA_FORMAT]);
		if (blockData != NULL && *(blockData + cookie->pos) == 0)
		{
			//NULL data, move to next block.
			release_block(ns->fd, cookie->block);
			totalRead += ns->logicalBlkSize[FS_DATA_FORMAT] - cookie->pos;
			cookie->pos = 0;
			cookie->block++;
		} 
		else break;
		if (totalRead >= cookie->totalSize) break;
	}
	
	cacheBlock = cookie->block;
	if (blockData != NULL && totalRead < cookie->totalSize)
	{
		vnode node;
							
		// JOLIET extension: ns->joliet_level
		if ((((result = InitNode(&node, blockData + cookie->pos, &bytesRead, ns->joliet_level))) == 
							B_NO_ERROR))
		{
			int nameBufSize = (bufsize - (2 * sizeof(dev_t) + 2* sizeof(ino_t) +
					sizeof(unsigned short)));

			buf->d_ino = (cookie->block << 30) +
							(cookie->pos & 0xFFFFFFFF);
			buf->d_reclen = node.fileIDLen;
			if (node.fileIDLen <= nameBufSize)
			{
				// need to do some size checking here.
				strncpy(buf->d_name, node.fileIDString, node.fileIDLen +1);
				//dprintf("ISOReadDirEnt  - success, name is %s\n", buf->d_name);
			}
			else
			{
				//dprintf("ISOReadDirEnt - ERROR, name %s does not fit in buffer of size %d\n", node.fileIDString, nameBufSize);
				result = EINVAL;
			}
			cookie->pos += bytesRead;
		}
		release_block(ns->fd, cacheBlock);
	}
	else 
	{
		if (totalRead >= cookie->totalSize) result = ENOENT;
		else result = ENOMEM;
	}
	
	//dprintf("ISOReadDirEnt - EXIT, result is %s, vnid is %Lu\n", strerror(result),buf->d_ino);
	return result;
}

int
InitVolDesc(nspace* vol, char* buf)
{
	//dprintf("InitVolDesc - ENTER\n");
	
	vol->volDescType = *(uint8*)buf++;
	
	vol->stdIDString[5] = '\0';
	strncpy(vol->stdIDString, buf, 5);
	buf += 5;
	
	vol->volDescVersion = *(uint8*)buf;
	buf += 2; // 8th byte unused
	
	vol->systemIDString[32] = '\0';
	strncpy(vol->systemIDString, buf, 32);
	buf += 32;
	//dprintf("InitVolDesc - system id string is %s\n", vol->systemIDString);
	
	vol->volIDString[32] = '\0';
	strncpy(vol->volIDString, buf, 32);
	buf += (32 + 80-73 + 1);	// bytes 80-73 unused
	//dprintf("InitVolDesc - volume id string is %s\n", vol->volIDString);

	// not so much a JOLIET extension, but a feature:
	// erase trailing spaces from discname
	{
		int i;
		for(i=31; i>0 && vol->volIDString[i]==' '; i--)
			vol->volIDString[i] = '\0';
	}
	
	vol->volSpaceSize[LSB_DATA] = *(uint32*)buf;
	buf += 4;
	vol->volSpaceSize[MSB_DATA] = *(uint32*)buf;
	buf+= (4 + 120-89 + 1); 		// bytes 120-89 unused
	
	vol->volSetSize[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->volSetSize[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->volSeqNum[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->volSeqNum[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->logicalBlkSize[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->logicalBlkSize[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->pathTblSize[LSB_DATA] = *(uint32*)buf;
	buf += 4;
	vol->pathTblSize[MSB_DATA] = *(uint32*)buf;
	buf += 4;
	
	vol->lPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->lPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->optLPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->optLPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->mPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->mPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	vol->optMPathTblLoc[LSB_DATA] = *(uint16*)buf;
	buf += 2;
	vol->optMPathTblLoc[MSB_DATA] = *(uint16*)buf;
	buf += 2;
	
	// Fill in directory record.
	// JOLIET extension: ,0
	InitNode(&(vol->rootDirRec), buf, NULL, 0);

	vol->rootDirRec.id = ISO_ROOTNODE_ID;
	buf += 34;
	
	vol->volSetIDString[28] = '\0';
	strncpy(vol->volSetIDString, buf, 28);
	buf += 28;
	//dprintf("InitVolDesc - volume set id string is %s\n", vol->volSetIDString);
	
	vol->pubIDString[128] = '\0';
	strncpy(vol->pubIDString, buf, 128);
	buf +=128;
	//dprintf("InitVolDesc - volume pub id string is %s\n", vol->pubIDString);
	
	vol->dataPreparer[128] = '\0';
	strncpy(vol->dataPreparer, buf, 128);
	buf += 128;
	//dprintf("InitVolDesc - volume dataPreparer string is %s\n", vol->dataPreparer);
	
	vol->appIDString[128] = '\0';
	strncpy(vol->appIDString, buf, 128);
	buf += 128;
	//dprintf("InitVolDesc - volume app id string is %s\n", vol->appIDString);
	
	vol->copyright[37] = '\0';
	strncpy(vol->copyright, buf, 37);
	buf += 38;
	//dprintf("InitVolDesc - copyright is %s\n", vol->copyright);
	
	vol->abstractFName[37] = '\0';
	strncpy(vol->abstractFName, buf, 37);
	buf += 38;
	
	vol->biblioFName[37] = '\0';
	strncpy(vol->biblioFName, buf, 37);
	buf += 38;
	
	InitVolDate(&(vol->createDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->modDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->expireDate), buf);
	buf += 17;
	
	InitVolDate(&(vol->effectiveDate), buf);
	buf += 17;
	
	vol->fileStructVers = *(uint8*)buf;
	return 0;
}

// JOLIET extension: uint8 joliet_level
int
InitNode(vnode* rec, char* buf, int* bytesRead, uint8 joliet_level)
{
	int 	result = B_NO_ERROR;
	uint8 	recLen = *(uint8*)buf++;
	bool    no_rock_ridge_stat_struct = TRUE;
	
	if (bytesRead != NULL) *bytesRead = recLen;

	//dprintf("InitNode - ENTER, bufstart is %lu, record length is %d bytes\n", buf, recLen);

	if (recLen > 0)
	{
		rec->extAttrRecLen = *(uint8*)(buf++);
		
		rec->startLBN[LSB_DATA] = *(uint32*)(buf);
		buf += 4;
		rec->startLBN[MSB_DATA] = *(uint32*)(buf);
		buf += 4;
		//dprintf("InitNode - data start LBN is %ld\n", rec->startLBN[FS_DATA_FORMAT]);
		
		rec->dataLen[LSB_DATA] = *(uint32*)(buf);
		buf += 4;
		rec->dataLen[MSB_DATA] = *(uint32*)(buf);
		buf += 4;
		//dprintf("InitNode - data length is %ld\n", rec->dataLen[FS_DATA_FORMAT]);
		
		InitRecDate(&(rec->recordDate), buf);
		buf += 7;
		
		rec->flags = *(uint8*) (buf );
		buf++;
		//dprintf("InitNode - flags are %d\n", rec->flags);
		
		rec->fileUnitSize = *(uint8*)(buf);
		buf++;
		//dprintf("InitNode - fileUnitSize is %d\n", rec->fileUnitSize);
		
		rec->interleaveGapSize = *(uint8*)(buf);
		buf ++;
		//dprintf("InitNode - interleave gap size = %d\n", rec->interleaveGapSize);

		rec->volSeqNum = *(uint32*)(buf);
		buf += 4;
		//dprintf("InitNode - volume seq num is %d\n", rec->volSeqNum);
			
		rec->fileIDLen = *(uint8*)(buf);
		buf ++;
		//dprintf("InitNode - file id length is %d\n", rec->fileIDLen);
		
		if (rec->fileIDLen > 0)
		{
			// JOLIET extension:
			// on joliet discs, buf[0] can be 0 for Unicoded filenames,
			// so I've added a check here to test explicitely for
			// directories (which have length 1)
			if (rec->fileIDLen == 1)
			{
				// Take care of "." and "..", the first two dirents are
				// these in iso.
				if (buf[0] == 0)
				{
					rec->fileIDString = strdup(".");
					rec->fileIDLen = 1;
				}
				else if (buf[0] == 1)
				{
					rec->fileIDString = strdup("..");
					rec->fileIDLen = 2;
				}
			}
			else
			{
				// JOLIET extension:
				// convert Unicode16 string to UTF8
				if (joliet_level > 0)
				{
					int i;

					rec->fileIDLen = rec->fileIDLen >> 1; // divide by 2 because Unicode is 16bit
					rec->fileIDString = (char*)malloc((rec->fileIDLen) + 1);
					if (rec->fileIDString != NULL)
					{
						// For the moment, let's assume each 16bit
						// Unicodechar has MSB=0 and LSB=ASCII-value
						// A Unicode-to-UTF8 conversion should be better...  GJvR
						for(i=0; i<rec->fileIDLen; i++)
						{
							rec->fileIDString[i] = buf[2*i + 1];
						}
						rec->fileIDString[rec->fileIDLen] = '\0';
					}
					else
					{
						// Error
						result = ENOMEM;
						//dprintf("InitNode - unable to allocate memory!\n");
					}
				}
				else
				{
					rec->fileIDString = (char*)malloc((rec->fileIDLen)+ 1);
					if (rec->fileIDString != NULL)
					{	
						strncpy(rec->fileIDString, buf, rec->fileIDLen);
						rec->fileIDString[rec->fileIDLen] = '\0';
					}
					else
					{
						// Error
						result = ENOMEM;
						//dprintf("InitNode - unable to allocate memory!\n");
					}
				}
			}
			//Get rid of semicolons, which are used to delineate file versions.q
			{
				char* semi = NULL;
				while ( (semi = strchr(rec->fileIDString, ';')) != NULL)
				{
					semi[0] = '\0';
				}
			
			}
			//dprintf("DirRec ID String is: %s\n", rec->fileIDString);

			if (result == B_NO_ERROR)
			{
				buf += rec->fileIDLen;
				if (!(rec->fileIDLen % 2)) buf++;

				// Now we're at the start of the rock ridge stuff
				{
					char*	altName = NULL;
					char*	slName = NULL;
					uint16	altNameSize = 0;
					uint16	slNameSize = 0;
					uint8	slFlags = 0;
					uint8	length = 0;
					bool 	done = FALSE;

					//dprintf("RR: Start of extensions, but at 0x%x\n", buf);
					//kernel_debugger("");
					
					memset(&(rec->attr.stat), 0, 2*sizeof(struct stat));
					
					// Set defaults, in case there is no RR stuff.
					rec->attr.stat[FS_DATA_FORMAT].st_mode = (S_IRUSR | S_IRGRP | S_IROTH);
					
					while (!done)
					{
						buf+= length;
						length = *(uint8*)(buf + 2);
						switch (0x100 * buf[0] + buf[1])
						{
							// Stat structure stuff
							case 'PX':
							{
								uint8 bytePos = 3;
								//dprintf("RR: found PX, length %u\n", length);
								rec->attr.pxVer = *(uint8*)(buf+bytePos++);
                                no_rock_ridge_stat_struct = FALSE;
								
								// st_mode
								rec->attr.stat[LSB_DATA].st_mode = *(mode_t*)(buf + bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_mode = *(mode_t*)(buf + bytePos);
								bytePos += 4;
								
								// st_nlink
								rec->attr.stat[LSB_DATA].st_nlink = *(nlink_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_nlink = *(nlink_t*)(buf + bytePos);
								bytePos += 4;
								
								// st_uid
								rec->attr.stat[LSB_DATA].st_uid = *(uid_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_uid = *(uid_t*)(buf+bytePos);
								bytePos += 4;
								
								// st_gid
								rec->attr.stat[LSB_DATA].st_gid = *(gid_t*)(buf+bytePos);
								bytePos += 4;
								rec->attr.stat[MSB_DATA].st_gid = *(gid_t*)(buf+bytePos);
								bytePos += 4;
							}	
							break;
							
							case 'PN':
								//dprintf("RR: found PN, length %u\n", length);
							break;
							
							// Symbolic link info
							case 'SL':
							{
								uint8	bytePos = 3;
								uint8	lastFlags = slFlags;
								uint8	lastCompFlag = 0;
								uint8	addPos = 0;
								bool	slDone = FALSE;
								bool	useSeparator = TRUE;
								
								//dprintf("RR: found SL, length %u\n", length);
								//dprintf("Buffer is at 0x%x\n", buf);
								//dprintf("Current length is %u\n", slNameSize);
								//kernel_debugger("");
								rec->attr.slVer = *(uint8*)(buf + bytePos++);
								slFlags = *(uint8*)(buf + bytePos++);
								
								//dprintf("sl flags are %u\n", slFlags);
								while (!slDone && bytePos < length)
								{
									uint8 compFlag = *(uint8*)(buf + bytePos++);
									uint8 compLen = *(uint8*)(buf + bytePos++);
									
									if (slName == NULL) useSeparator = FALSE;
									
									addPos = slNameSize;
									
									//dprintf("sl comp flags are %u, length is %u\n", compFlag, compLen);
									//dprintf("Current name size is %u\n", slNameSize);
									switch (compFlag)
									{
										case SLCP_CONTINUE:
											useSeparator = FALSE;
										default:
											// Add the component to the total path.
											slNameSize += compLen;
											if ( useSeparator ) slNameSize++;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											
											if (useSeparator) 
											{
												//dprintf("Adding separator\n");
												slName[addPos++] = '/';
											}	
											
											//dprintf("doing memcopy of %u bytes at offset %d\n", compLen, addPos );
											memcpy((slName + addPos), (buf + bytePos), compLen);
											
											addPos += compLen;
											useSeparator = TRUE;
										break;
										
										case SLCP_CURRENT:
											//dprintf("InitNode - found link to current directory\n");
											slNameSize += 2;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "./", 2);
											useSeparator = FALSE;
										break;
										
										case SLCP_PARENT:
											slNameSize += 3;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "../", 3);
											useSeparator = FALSE;
										break;
										
										case SLCP_ROOT:
											//dprintf("InitNode - found link to root directory\n");
											slNameSize += 1;
											if (slName == NULL) 
												slName = (char*)malloc(slNameSize + 1);
											else
												slName = (char*)realloc(slName, slNameSize + 1);
											memcpy(slName + addPos, "/", 1);
											useSeparator = FALSE;
										break;
										
										case SLCP_VOLROOT:
											slDone = TRUE;
										break;
										
										case SLCP_HOST:
											slDone = TRUE;
										break;
									}
									slName[slNameSize] = '\0';
									lastCompFlag = compFlag;
									bytePos += compLen;
									//dprintf("Current sl name is \'%s\'\n", slName);
								}
							}
							rec->attr.slName = slName;
							//dprintf("InitNode = symlink name is \'%s\'\n", slName);
							break;
							// Altername name
							case 'NM':
							{
								uint8	bytePos = 3;
								uint8	flags = 0;
								uint16	oldEnd = altNameSize;
								
								altNameSize += length - 5;
								if (altName == NULL) altName = (char*)malloc(altNameSize + 1);
								else
									altName = (char*)realloc(altName, altNameSize + 1);
								
								//dprintf("RR: found NM, length %u\n", length);
								// Read flag and version.
								rec->attr.nmVer = *(uint8*)(buf + bytePos++);
								flags = *(uint8*)(buf + bytePos++);
							
								//dprintf("RR: nm buf is %s, start at 0x%x\n", (buf + bytePos), buf+bytePos);
								//kernel_debugger("");
	
								// Build the file name.
								memcpy(altName + oldEnd, buf + bytePos, length - 5);
								altName[altNameSize] = '\0';
								//dprintf("RR: alt name is %s\n", altName);
								
								// If the name is not continued in another record, update
								// the record name.
								if (! (flags & NM_CONTINUE))
								{
									// Get rid of the ISO name, replace with RR name.
									if (rec->fileIDString != NULL) free(rec->fileIDString);
									rec->fileIDString = altName;
									rec->fileIDLen = altNameSize;
								}
							}
							break;
							
							// Deep directory record masquerading as a file.
							case 'CL':
							
								//dprintf("RR: found CL, length %u\n", length);
								rec->flags |= ISO_ISDIR;
								rec->startLBN[LSB_DATA] = *(uint32*)(buf+4);
								rec->startLBN[MSB_DATA] = *(uint32*)(buf+8);
								
							break;
							
							case 'PL':
								//dprintf("RR: found PL, length %u\n", length);
							break;
							
							// Relocated directory, we should skip.
							case 'RE':
								result = EINVAL;
								//dprintf("RR: found RE, length %u\n", length);
							break;
							
							case 'TF':
								//dprintf("RR: found TF, length %u\n", length);
							break;
							
							case 'RR':
								//dprintf("RR: found RR, length %u\n", length);
							break;
							
							default:
								//dprintf("RR: End of extensions.\n");
								done = TRUE;
							break;
						}
					}
				}
			}
		}
		else
		{
			//dprintf("InitNode - File ID String is 0 length\n");
			result = ENOENT;
		}
	}
	else result = ENOENT;
	//dprintf("InitNode - EXIT, result is %s name is \'%s\'\n", strerror(result),rec->fileIDString );
	
	
	if (no_rock_ridge_stat_struct) {
		if (rec->flags & ISO_ISDIR)
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH);
		else
			rec->attr.stat[FS_DATA_FORMAT].st_mode |= (S_IFREG);
	}

	return result;
}

static int
InitVolDate(ISOVolDate* date, char* buf)
{
	memcpy(date, buf, 17);
	return 0;
}

static int
InitRecDate(ISORecDate* date, char* buf)
{
	memcpy(date, buf, 7 );
	return 0;
}

int
ConvertRecDate(ISORecDate* inDate, time_t* outDate)
{
	time_t	time;
	int		days, i, year, tz;
	
	year = inDate->year -70;
	tz = inDate->offsetGMT;
	
	if (year < 0)
	{
		time = 0;
	}
	else
	{
		int monlen[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		days = (year * 365);
		if (year > 2)
		{
			days += (year + 1)/ 4;
		}
		for (i = 1; i < inDate->month; i++)
		{
			days += monlen[i-1];
		}
		if (((year + 2) % 4) == 0 && inDate->month > 2)
		{
			days++;
		}
		days += inDate->date - 1;
		time = ((((days*24) + inDate->hour) * 60 + inDate->minute) * 60)
					+ inDate->second;
		if (tz & 0x80)
		{
			tz |= (-1 << 8);	
		}
		if (-48 <= tz && tz <= 52)
			time += tz *15 * 60;
	}
	*outDate = time;
	return 0;
}
