typedef long long dr9_off_t;
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <KernelExport.h>
#include <time.h>
#include <fsproto.h>
#include <lock.h>
#include <cache.h>
#include <malloc.h>
//#include "dmalloc.h"

#include "iso.h"


/*  Start of fundamental (read-only) required functions */
static int		fs_mount(nspace_id nsid, const char *device, ulong flags,
						void *parms, size_t len, void **data, vnode_id *vnid);
static int		fs_unmount(void *_ns);

static int		fs_walk(void *_ns, void *_base, const char *file,
						char **newpath, vnode_id *vnid);

static int		fs_read_vnode(void *_ns, vnode_id vnid, char r, void **node);
static int		fs_write_vnode(void *_ns, void *_node, char r);
static int		fs_rstat(void *_ns, void *_node, struct stat *st);
static int		fs_open(void *_ns, void *_node, int omode, void **cookie);
static int		fs_read(void *_ns, void *_node, void *cookie, off_t pos,
						void *buf, size_t *len);
/// fs_free_cookie - free cookie for file created in open.
static int		fs_free_cookie(void *ns, void *node, void *cookie);
static int		fs_close(void *ns, void *node, void *cookie);

// fs_access - checks permissions for access.
static int		fs_access(void *_ns, void *_node, int mode);

// fs_opendir - creates fs-specific "cookie" struct that can tell where
//					we are at in the directory list.
static int		fs_opendir(void* _ns, void* _node, void** cookie);
// fs_readdir - read 1 or more dirents, keep state in cookie, return
//					0 when no more entries.
static int		fs_readdir(void *_ns, void *_node, void *cookie,
					long *num, struct dirent *buf, size_t bufsize);
// fs_rewinddir - set cookie to represent beginning of directory, so
//					later fs_readdir calls start at beginning.
static int		fs_rewinddir(void *_ns, void *_node, void *cookie);
// fs_closedir - Do whatever you need to to close a directory (sometimes
//					nothing), but DON'T free the cookie!
static int		fs_closedir(void *_ns, void *_node, void *cookie);
// fs_fee_dircookie - Free the fs-specific cookie struct
static int		fs_free_dircookie(void *_ns, void *_node, void *cookie);

// fs_rfsstat - Fill in fs_info struct for device.
static int		fs_rfsstat(void *_ns, struct fs_info *);

// fs_readlink - Read in the name of a symbolic link.
static int 		fs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize);
/* End of fundamental (read-only) required functions. */

#if 0
static int		fs_remove_vnode(void *ns, void *node, char r);
static int		fs_secure_vnode(void *ns, void *node);
static int		fs_create(void *ns, void *dir, const char *name,
					int perms, int omode, vnode_id *vnid, void **cookie);
static int		fs_mkdir(void *ns, void *dir, const char *name, int perms);
static int		fs_unlink(void *ns, void *dir, const char *name);
static int		fs_rmdir(void *ns, void *dir, const char *name);
static int		fs_wstat(void *ns, void *node, struct stat *st, long mask);
static int		fs_write(void *ns, void *node, void *cookie, off_t pos,
						const void *buf, size_t *len);
static int		fs_ioctl(void *ns, void *node, void *cookie, int cmd,
						void *buf, size_t len);
static int		fs_wfsstat(void *ns, struct fs_info *);
static int		fs_sync(void *ns);
static int     	fs_initialize(const char *devname, void *parms, size_t len);
#endif 



/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/

vnode_ops fs_entry =  {
	&fs_read_vnode,						// read_vnode func ptr
	&fs_write_vnode,					// write_vnode func ptr
	NULL, 								// remove_vnode func ptr
	NULL,								// secure_vnode func ptr
	&fs_walk,							// walk func ptr
	&fs_access,							// access func ptr
	NULL, 								// create func ptr
	NULL, 								// mkdir func ptr
	NULL,
	NULL,
	NULL,
	NULL, 								// unlink func ptr
	NULL, 								// rmdir func ptr
	&fs_readlink,						// readlink func ptr
	&fs_opendir,						// opendir func ptr
	&fs_closedir,						// closedir func ptr
	&fs_free_dircookie,					// free_dircookie func ptr
	&fs_rewinddir,						// rewinddir func ptr
	&fs_readdir,						// readdir func ptr
	&fs_open,							// open file func ptr
	&fs_close,							// close file func ptr
	&fs_free_cookie,					// free cookie func ptr
	&fs_read,							// read file func ptr
	NULL, 								// write file func ptr
	NULL, /* readv */
	NULL, /* writev */
	NULL,								// ioctl func ptr
	NULL,								// setflags file func ptr
	&fs_rstat,							// rstat func ptr
	NULL, 								// wstat func ptr
	NULL,
	NULL, 								// initialize func ptr
	&fs_mount,							// mount func ptr
	&fs_unmount,						// unmount func ptr
	NULL,								// sync func ptr
	&fs_rfsstat,						// rfsstat func ptr
	NULL,								// wfsstat func ptr
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int32	api_version = B_CUR_FS_API_VERSION;


static char* 	gFSName = "ISO9660";	// replace w/ your fs name.

thread_id gSerializeThread;

static int 
fs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **data, vnode_id *vnid)
{
	/*
	Kernel passes in nspace_id, (representing a disk or partition?)
	and a string representing the device (eg, "/dev/scsi/disk/030/raw)
	Flags will be used for things like specifying read-only mounting.
	parms is parameters passed in as switches from the mount command, 
	and len is the length of the otions. data is a pointer to a 
	driver-specific struct that should be allocated in this routine. 
	It will then be passed back in by the kernel to a number of the other 
	fs driver functions. vnid should also be passed back to the kernel, 
	representing the vnode id of the root vnode.
	*/
	#pragma unused(flags)
	#pragma unused(parms)
	#pragma unused (len)
	// return EINVAL if it's not a device compatible with the driver.
	int 		result = EINVAL;
	
	/* Replace this with your own internally-defined struct that represents
		a volume. */
	nspace*		vol;
	
	//dprintf("fs_mount - ENTER\n");
	/* Create semaphore if it's not already created. When do we need to
		use semaphores? */	
	//dprintf("%s: nsid %d device %s\n",gFSName, nsid,device);


	// Try and mount volume as an ISO volume.
	result = ISOMount(device, O_RDONLY, &vol);
	
	// If it is ISO …
	if (result == B_NO_ERROR)
	{
		//vnode_id rootID = vol->rootDirRec.startLBN[FS_DATA_FORMAT];
		//*vnid = rootID;
		*vnid = ISO_ROOTNODE_ID;
		*data = (void*)vol;
		
		vol->id = nsid;
		
		// You MUST do this. Create the vnode for the root.
		result = new_vnode(nsid, *vnid, (void*)&(vol->rootDirRec));
		if (result != B_NO_ERROR)
		{
			remove_cached_device_blocks(vol->fd, 0);
			free(vol);
			result = EINVAL;
		}
		else 
		{
			result = B_NO_ERROR;	
		}		
	}
	//dprintf("fs_mount - EXIT, result code is %s\n", strerror(result));
	return result;
}

static int		
fs_unmount(void *_ns)
{
	int result = B_NO_ERROR;
	
	nspace* ns = (nspace*)_ns;
	//dprintf("fs_unmount - ENTER\n");
	//dprintf("fs_unmount - removing cached blocks\n");
	remove_cached_device_blocks(ns->fd, 0);
	//dprintf("fs_unmount - closing volume\n");
	result = close(ns->fd);
	//dprintf("fs_unmount - freeing volume data\n");
	free(ns);
	//dprintf("fs_unmount - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_rfsstat - Fill in fs_info struct for device.
static int		
fs_rfsstat(void *_ns, struct fs_info * fss)
{
	nspace* ns = (nspace*)_ns;
	
	//dprintf("fs_rfsstat - ENTER\n");
	
	// Fill in device id.
	//fss->dev = ns->fd;
	
	// Root vnode ID
	//fss->root = ISO_ROOTNODE_ID;
	
	// File system flags.
	fss->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
	
	// FS block size.
	fss->block_size = ns->logicalBlkSize[FS_DATA_FORMAT];
	
	// IO size - specifies buffer size for file copying
	fss->io_size = 65536;
	
	// Total blocks?
	fss->total_blocks = ns->volSpaceSize[FS_DATA_FORMAT];
	
	// Free blocks = 0, read only
	fss->free_blocks = 0;
	
	// Device name.
	strncpy(fss->device_name, ns->devicePath, sizeof(fss->device_name));

	strncpy(fss->volume_name, ns->volIDString, sizeof(fss->volume_name));
	
	// File system name
	strcpy(fss->fsh_name,"ISO9660");
	
	//dprintf("fs_rfsstat - EXIT\n");
	return 0;
}

/* fs_walk - the walk function just "walks" through a directory looking for
	the specified file. When you find it, call get_vnode on its vnid to init
	it for the kernel.
*/

static int		
fs_walk(void *_ns, void *base, const char *file, char **newpath, 
			vnode_id *vnid)
{
	/* Starting at the base, find file in the subdir, and return path
		string and vnode id of file. */
	nspace*		ns = (nspace*)_ns;
	vnode* 		baseNode = (vnode*)base;
	uint32		dataLen = baseNode->dataLen[FS_DATA_FORMAT];
	vnode*		newNode = NULL;
	int			result = ENOENT;
	bool		done = FALSE;
	uint32		totalRead = 0;
	dr9_off_t	block;
	
	block = baseNode->startLBN[FS_DATA_FORMAT];
	
	//dprintf("fs_walk - ENTER\n");
	//dprintf("fs_walk - looking for %s in dir file of length %d\n", file,baseNode->dataLen[FS_DATA_FORMAT]);
	
	if (strcmp(file,".") == 0) 
	{
		//dprintf("fs_walk - found \".\" file.\n");
		*vnid = baseNode->id;
		if (get_vnode(ns->id,*vnid,(void **)&newNode) != 0)
	    		result = EINVAL;
	    else
	    {
	    	result = B_NO_ERROR;
	    }
	}
	else if (strcmp(file, "..") == 0)
	{
		//dprintf("fs_walk - found \"..\" file.\n");
		*vnid = baseNode->parID;
		if (get_vnode(ns->id, *vnid, (void**)&newNode) != 0)
			result = EINVAL;
		else
		{
			result = B_NO_ERROR;
		}
	}

	else 
	{
		char* blockData;
		while ((totalRead < dataLen) && !done) 
		{
			dr9_off_t	cachedBlock = block;
			
			blockData = (char*)get_block(ns->fd, block,  ns->logicalBlkSize[FS_DATA_FORMAT]);
			if (blockData != NULL)
			{
				int				bytesRead = 0;
				dr9_off_t		blockBytesRead = 0;
				vnode 			node;
				int				initResult;
				//dprintf("fs_walk - read buffer from disk at LBN %Ld into buffer 0x%x.\n", block, blockData);
				//kernel_debugger("");
				// Move to the next 2-block set if necessary				
				// Don't go over end of buffer, if dir record sits on boundary.
			
				node.fileIDString = NULL;
				node.attr.slName = NULL;
				
				while (blockBytesRead  < 2*ns->logicalBlkSize[FS_DATA_FORMAT] && 
						 (totalRead + blockBytesRead < dataLen) &&
						 blockData[0] != 0 &&
						 !done)
				{
					// JOLIET extension: ns->joliet_level
					initResult = InitNode(&node, blockData, &bytesRead, ns->joliet_level);
					//dprintf("fs_walk - InitNode returned %s, filename %s, %d bytes read\n", strerror(initResult), node.fileIDString, bytesRead);
					
					if (initResult == B_NO_ERROR)
					{
						if ((strlen(node.fileIDString) == strlen(file)) &&
								!strncmp(node.fileIDString, file, strlen(file)))
						{
							//dprintf("fs_walk - success, found vnode at block %Ld, pos %Ld\n", block, blockBytesRead);
							*vnid = (block << 30) + (blockBytesRead & 0xFFFFFFFF);
							//dprintf("fs_walk - New vnode id is %Ld\n", *vnid);
							if (get_vnode(ns->id,*vnid,(void **)&newNode) != 0)
								result = EINVAL;
							else 
							{
								newNode->parID = baseNode->id;
								done = TRUE;
								result = B_NO_ERROR;
							}
						}
						else
						{
							if (node.fileIDString != NULL)
							{
								free(node.fileIDString);
								node.fileIDString = NULL;
							}
							if (node.attr.slName != NULL)
							{
								free(node.attr.slName);
								node.attr.slName = NULL;
							}
						}
					}
					else
					{	
						result = initResult;
						if (bytesRead == 0) done = TRUE;
					}
					blockData += bytesRead;
					blockBytesRead += bytesRead;
					//dprintf("fs_walk - Adding %d bytes to blockBytes read (total %Ld/%Ld).\n", bytesRead, blockBytesRead, baseNode->dataLen[FS_DATA_FORMAT]);
				}
				totalRead += ns->logicalBlkSize[FS_DATA_FORMAT];
				block++;
				//dprintf("fs_walk - moving to next block %Ld, total read %Ld\n", block, totalRead);
				release_block(ns->fd, cachedBlock);

			}
			else done = TRUE;
		}
		
		// Check to see if vnode is a symbolic link. If so, fill in the newpath variable
		// with the path to the real file, and call put_vnode.
		if (newNode != NULL)
		{
			if ( S_ISLNK(newNode->attr.stat[FS_DATA_FORMAT].st_mode) && newpath != NULL)
			{
				//dprintf ("fs_walk - symbolic link file \'%s\' requested.\n", newNode->attr.slName);
				result = new_path(newNode->attr.slName, newpath);
				//dprintf ("fs_walk - putting vnode.\n");
				put_vnode(ns->id, *vnid);
			}
		}
	}
	//dprintf("fs_walk - EXIT, result is %s, vnid is %Lu\n", strerror(result), *vnid);
	return result;
}

// fs_read_vnode - Using vnode id, read in vnode information into fs-specific struct,
//					and return it in node. the reenter flag tells you if this function
//					is being called via some other fs routine, so that things like 
//					double-locking can be avoided.
static int		
fs_read_vnode(void *_ns, vnode_id vnid, char reenter, void **node)
{
	#pragma unused (reenter)

	uint32 		block, pos;
	nspace*		ns = (nspace*)_ns;
	int			result = B_NO_ERROR;
	vnode* 		newNode = (vnode*)calloc(sizeof(vnode), 1);
	
	//dprintf("fs_read_vnode - ENTER, block = %ld, pos = %ld, raw = %Lu node 0x%x\n", block, pos, vnid, newNode);
	pos = (vnid & 0x3FFFFFFF);
	block = (vnid >> 30);
	
	if (newNode != NULL)
	{
		if (vnid == ISO_ROOTNODE_ID)
		{
			//dprintf("fs_read_vnode - root node requested.\n");
			memcpy(newNode, &(ns->rootDirRec), sizeof(vnode));
			*node = (void*)newNode;
		}
		else
		{
			dr9_off_t cachedBlock = block;
			char* blockData = (char*)get_block(ns->fd, block, ns->logicalBlkSize[FS_DATA_FORMAT]);
			if (pos > ns->logicalBlkSize[FS_DATA_FORMAT]) {
				result = EINVAL;
		 	} else if (blockData != NULL) {
		 		// JOLIET extension: ns->joliet_level
				result = InitNode(newNode, blockData + pos, NULL, ns->joliet_level);
				newNode->id = vnid;
				//dprintf("fs_read_vnode - init result is %s\n", strerror(result));
								*node = (void*)newNode;
				//dprintf("fs_read_vnode - new file %s, size %ld\n", newNode->fileIDString, newNode->dataLen[FS_DATA_FORMAT]);
				release_block(ns->fd, cachedBlock);
			}
		}
	}
	else result = ENOMEM;
	
	//dprintf("fs_read_vnode - EXIT, result is %s\n", strerror(result));
	return result;
}

static int		
fs_write_vnode(void *ns, void *_node, char reenter)
{
	#pragma unused (ns)
	#pragma unused (reenter)
	
	int 		result = B_NO_ERROR;
	vnode* 		node = (vnode*)_node;
	
	//dprintf("fs_write_vnode - ENTER (0x%x)\n", node);
	if (node != NULL)
	{ 
		if (node->id != ISO_ROOTNODE_ID)
		{
			if (node->fileIDString != NULL) free (node->fileIDString);
			if (node->attr.slName != NULL) free (node->attr.slName);
			free(node);
		}
	}
	//dprintf("fs_write_vnode - EXIT\n");
	return result;
}

// fs_rstat - fill in stat struct
static int		
fs_rstat(void *_ns, void *_node, struct stat *st)
{
	nspace* ns = (nspace*)_ns;
	vnode*	node = (vnode*)_node;
	int 	result = B_NO_ERROR;
	time_t	time;
	
	//dprintf("fs_rstat - ENTER\n");
	st->st_dev = ns->id;
	st->st_ino = node->id;
	st->st_nlink = node->attr.stat[FS_DATA_FORMAT].st_nlink;
	st->st_uid = node->attr.stat[FS_DATA_FORMAT].st_uid;
	st->st_gid = node->attr.stat[FS_DATA_FORMAT].st_gid;
	st->st_blksize = 65536;
	st->st_mode = node->attr.stat[FS_DATA_FORMAT].st_mode;
	
	// Same for file/dir in ISO9660
	st->st_size = node->dataLen[FS_DATA_FORMAT];
	if (ConvertRecDate(&(node->recordDate), &time) == B_NO_ERROR) 
	st->st_ctime = st->st_mtime = st->st_atime = time;
	//dprintf("fs_rstat - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_open - Create a vnode cookie, if necessary, to use when
// 				reading/writing a file
static int		
fs_open(void *_ns, void *_node, int omode, void **cookie)
{
	#pragma unused (_ns)
	#pragma unused (cookie)
	int		result = B_NO_ERROR;
	//if (omode & O_WRONLY) result = EPERM;
	//else
	{	
		vnode* 	node = (vnode*)_node;
	
		if ((node->flags & ISO_ISDIR)) result = EISDIR;
		//dprintf("fs_open - ENTER\n");
		// No cookie info needed, just return.
		//dprintf("fs_open - EXIT\n");
	}
	return result;
}

// fs_read
// Read a file specified by node, using information in cookie
// and at offset specified by pos. read len bytes into buffer buf.
static int		
fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, 
			size_t *len)
{
	#pragma unused (cookie)
	
	nspace* 	ns = (nspace*)_ns;			// global stuff
	vnode* 		node = (vnode*)_node;		// The read file vnode.
	uint16		blockSize = ns->logicalBlkSize[FS_DATA_FORMAT];
	uint32	 	startBlock = node->startLBN[FS_DATA_FORMAT]  + 
							(pos / blockSize);
	dr9_off_t	blockPos = pos %blockSize;
	dr9_off_t 	numBlocks = 0;
	uint32		dataLen = node->dataLen[FS_DATA_FORMAT];
	int		result = B_NO_ERROR;
	size_t		endLen = 0;
	size_t		reqLen = *len;
	size_t		startLen =  0;

	if (pos < 0) pos = 0;
	*len = 0;	

	// If passed-in requested length is bigger than file size, change it to
	// file size.
	if (reqLen + pos > dataLen)
	{
		reqLen = dataLen - pos;
	}
	// Compute the length of the partial start-block read, if any.
	
	if (reqLen + blockPos <= blockSize)
	{
		startLen = reqLen;
	}
	else if (blockPos > 0)
	{
		startLen = blockSize - blockPos;
	}

	if (blockPos == 0 && reqLen >= blockSize)
	{
		//dprintf("Setting startLen to 0, even block read\n");
		startLen = 0;
	}
	// Compute the length of the partial end-block read, if any.
	if (reqLen + blockPos > blockSize)
	{
		endLen = (reqLen +blockPos) % blockSize;
	}

	// Compute the number of middle blocks to read.
	numBlocks = ((reqLen - endLen - startLen) /  blockSize);
	
	
	//dprintf("fs_read - ENTER, pos is %Ld, len is %lu\n", pos, reqLen);
	//dprintf("fs_read - filename is %s\n", node->fileIDString);
	//dprintf("fs_read - total file length is %lu\n", dataLen);
	//dprintf("fs_read - start block of file is %lu\n", node->startLBN[FS_DATA_FORMAT]);
	//dprintf("fs_read - block pos is %Lu\n", blockPos);
	//dprintf("fs_read - read block will be %lu\n", startBlock);
	//dprintf("fs_read - startLen is %lu\n", startLen);
	//dprintf("fs_read - endLen is %lu\n", endLen);
	//dprintf("fs_read - num blocks to read is %Ld\n", numBlocks);
	
	*len = 0;
	// If pos >= file length, return length of 0.
	if (pos >= dataLen)
	{
		//dprintf("fs_read - End of file reached\n");
	}
	else
	{
		// Read in the first, potentially partial, block.
		if (startLen > 0)
		{
			dr9_off_t	cachedBlock = startBlock;
			char*		blockData = (char*)get_block(ns->fd, startBlock, blockSize);
			//dprintf("fs_read - getting block %lu\n", startBlock);
			if (blockData != NULL)
			{
				//dprintf("fs_read - copying first block, len is %d.\n", startLen);
				memcpy(buf, blockData+blockPos, startLen);
				*len += startLen;
				release_block(ns->fd, cachedBlock);
				startBlock++;	
			}
			else result = EIO;
		}

		// Read in the middle blocks.
		if (numBlocks > 0 && result == B_NO_ERROR)
		{
			//dprintf("fs_read - getting middle blocks\n");
			result = cached_read(ns->fd, startBlock, 
						((char*)buf) + startLen, 
						numBlocks, 
						blockSize);
			if (result == B_NO_ERROR)
			{
				*len += blockSize * numBlocks;
			}
		}

		// Read in the last partial block.
		if (result == B_NO_ERROR && endLen > 0)
		{
			dr9_off_t	endBlock = startBlock + numBlocks;
			char*		endBlockData = (char*)get_block(ns->fd, endBlock, blockSize);
			//dprintf("fs_read - getting end block\n");
			if (endBlockData != NULL)
			{
				char* endBuf = ((char*)buf) + (reqLen - endLen);
				
				memcpy(endBuf, endBlockData, endLen);
				release_block(ns->fd, endBlock);
				*len += endLen;
			}
			else result = EIO;
		}
	}
	//dprintf("fs_read - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_close - Do whatever is necessary to close a file, EXCEPT for freeing
//				the cookie!
static int		
fs_close(void *ns, void *node, void *cookie)
{
	#pragma unused (ns)
	#pragma unused (node)
	#pragma unused (cookie)
	//dprintf("fs_close - ENTER\n");
	//dprintf("fs_close - EXIT\n");
	return 0;
}

static int		
fs_free_cookie(void *ns, void *node, void *cookie)
{
	#pragma unused (ns)
	#pragma unused (node)
	#pragma unused (cookie)
	// We don't allocate file cookies, so we do nothing here.
	//dprintf("fs_free_cookie - ENTER\n");
	//if (cookie != NULL) free (cookie);
	//dprintf("fs_free_cookie - EXIT\n");
	return 0;
}

// fs_access - checks permissions for access.
static int		
fs_access(void *ns, void *node, int mode)
{
	#pragma unused (ns)
	#pragma unused (node)
	#pragma unused (mode)
	// ns 	- global, fs-specific struct for device
	// node	- node to check permissions for
	// mode - requested permissions on node.
	//dprintf("fs_access - ENTER\n");
	//dprintf("fs_access - EXIT\n");
	return 0;
}

static int
fs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize)
{
	#pragma unused(_ns)
	int		result = EINVAL;
	vnode*	node = (vnode*)_node;
	size_t	length;
	
	if (S_ISLNK(node->attr.stat[FS_DATA_FORMAT].st_mode))
	{
		length = strlen(node->attr.slName);
		if (length > *bufsize)
		{
			memcpy(buf, node->attr.slName, length);
		}
		else
		{
			memcpy(buf, node->attr.slName, length);
		}
		result = B_NO_ERROR;
	}
	*bufsize = length;
	return result;
}

// fs_opendir - creates fs-specific "cookie" struct that keeps track of where
//					you are at in reading through directory entries in fs_readdir.
static int		
fs_opendir(void *_ns, void *_node, void **cookie)
{
	// ns 		- global, fs-specific struct for device
	// node		- directory to open
	// cookie	- allocate fs-specific cookie and return here to
	//				maintain dirent position info for reads.	
	nspace* 				ns = (nspace*)_ns;
	vnode*					node = (vnode*)_node;
	int						result = B_NO_ERROR;
	dircookie* 				dirCookie = (dircookie*)malloc(sizeof(dircookie));

	//dprintf("fs_opendir - ENTER, node is 0x%x\n", _node); 
	if (!(node->flags & ISO_ISDIR)) result = EMFILE;
	if (dirCookie != NULL)
	{
		dirCookie->startBlock = node->startLBN[FS_DATA_FORMAT];
		dirCookie->block = node->startLBN[FS_DATA_FORMAT];
		dirCookie->totalSize = node->dataLen[FS_DATA_FORMAT];
		dirCookie->pos = 0;
		dirCookie->id = node->id;
		*cookie = (void*)dirCookie;
	}
	else
	{
		// Mem error
		result = ENOMEM;
	}
	//dprintf("fs_opendir - EXIT\n");
	return result;
}

// fs_readdir - read 1 or more dirents, keep state in cookie, return
//					0 when no more entries.
static int		
fs_readdir(void *_ns, void *_node, void *_cookie, long *num, 
			struct dirent *buf, size_t bufsize)
{
	int 		result = B_NO_ERROR;
	vnode* 		node = (vnode*)_node;
	nspace* 	ns = (nspace*)_ns;
	dircookie* 	dirCookie = (dircookie*)_cookie;
	
	//dprintf("fs_readdir - ENTER\n");
	result = ISOReadDirEnt(ns, dirCookie, buf, bufsize);
	
	// If we succeeded, return 1, the number of dirents we read.
	if (result == B_NO_ERROR)
		*num = 1;
	else *num = 0;
	
	// When you get to the end, don't return an error, just return
	// a zero in *num.
	
	if (result == ENOENT) result = B_NO_ERROR;
	//dprintf("fs_readdir - EXIT, result is %s\n", strerror(result));
	return result;
}
			
// fs_rewinddir - set cookie to represent beginning of directory, so
//					later fs_readdir calls start at beginning.
static int		
fs_rewinddir(void *ns, void *node, void* _cookie)
{
	#pragma unused (ns)
	#pragma unused (node)
	
	// ns 		- global, fs-specific struct for device
	// node		- directory to rewind
	// cookie	- current cookie for directory.	
	int			result = EINVAL;
	dircookie*	cookie = (dircookie*)_cookie;
	//dprintf("fs_rewinddir - ENTER\n");
	if (cookie != NULL)
	{
		cookie->block = cookie->startBlock;
		cookie->pos = 0;
		result = B_NO_ERROR;
	}
	//dprintf("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	return result;
}

// fs_closedir - Do whatever you need to to close a directory (sometimes
//					nothing), but DON'T free the cookie!
static int		
fs_closedir(void *ns, void *node, void *cookie)
{
	#pragma unused (ns)
	#pragma unused (node)
	#pragma unused (cookie)
	// ns 		- global, fs-specific struct for device
	// node		- directory to close
	// cookie	- current cookie for directory.	
	//dprintf("fs_closedir - ENTER\n");
	//dprintf("fs_closedir - EXIT\n");
	return 0;
}

// fs_free_dircookie - Free the fs-specific cookie struct
static int		
fs_free_dircookie(void *ns, void *node, void *cookie)
{
	#pragma unused (ns)
	#pragma unused (node)
	// ns 		- global, fs-specific struct for device
	// node		- directory related to cookie
	// cookie	- current cookie for directory, to free.	
	//dprintf("fs_free_dircookie - ENTER\n");
	if (cookie != NULL)
	{
		free(cookie);
	}
	//dprintf("fs_free_dircookie - EXIT\n");
	return 0;
}
