//#include <types.h>
#include <string.h>
#include <ctype.h>
//#include <sys/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/stat.h>

#include <user/sys_phantom.h>


#include "file_utils.h"
#include "statements.h"
#include "shell_defs.h"

int combine_path(const char *path1,const char *path2,char *out,unsigned int max_len)
{
    unsigned int cur_len = 0;

    cur_len = strlen(path1);
    if(cur_len > max_len) return 0;
    strcpy(out,path1);

    if(*path1 != 0){
        if(out[cur_len-1] != '/'){
            if(cur_len >= max_len) return 0;
            out[cur_len] = '/';
            out[cur_len+1] = 0;
            cur_len++;
        }
    }
    if(strlen(path2)+cur_len > max_len) return 0;
    strcat(out,path2);
    return 1;
}

int exists_file(const char *file_name)
{
    int handle = open(file_name,0);
    int exists;
    exists = (handle >= 0);
    if(exists) close(handle);
    return exists;
}

int find_file_in_path(const char *file_name,char *found_name,unsigned int max_size)
{
    char path[SCAN_SIZE+1];
    int  cnt=0;

    if(strchr(file_name,'/') != NULL){
        strncpy(found_name,file_name,max_size);
        found_name[max_size] = 0;
        if(exists_file(found_name)) return 1;
        found_name[0] = 0;
        return 0;
    }
    while(get_path(cnt,path,SCAN_SIZE)){
        if(combine_path(path,file_name,found_name,max_size)){
            if(exists_file(found_name)) return 1;
        }
        cnt++;
    }
    found_name[0] =0;
    return(0);
}


int exec_file(int argc, const char *argv[],int *retcode)
{
    (void) argc;

    char filename[255];
    int pid;

    if( !find_file_in_path(argv[0],filename,SCAN_SIZE-1)) return SHE_FILE_NOT_FOUND;

    /*
    pid = _kern_proc_create_proc(filename,filename, argv, argc, 5, 0);

    if(pid < 0) return SHE_CANT_EXECUTE;

    _kern_proc_wait_on_proc(pid, retcode);
    */

    pid = phantom_run( filename, argv, 0, P_RUN_WAIT );
    if(pid < 0) return SHE_CANT_EXECUTE;

    // TODO wait and get rc
    *retcode = 0;

    return SHE_NO_ERROR;
}


int read_file_in_buffer(const char *filename,char **buffer)
{
    struct stat istat;
    int file_no;
    int err;
    int size;

    *buffer = NULL;

    err = stat(filename,&istat);
    if(err < 0) return err;

    *buffer = malloc(istat.st_size+1);
    if(*buffer == 0) return -ENOMEM;

    file_no = open(filename,0);

    if(file_no < 0){
        free(*buffer);
        return file_no;
    }

    size = read(file_no, *buffer, istat.st_size );

    close(file_no);

    if(size < 0) free(*buffer);

    return size;
}
