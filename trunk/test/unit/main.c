#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <CUnit/Basic.h>

#include "utils.h"

int modules_alphasort(const char **a, const char **b) {
    return strcoll(*a, *b);
}

void (*runSuite)( void);

/* Функция поиска динамических модулей  */
size_t searchModulesInDir( char ***m_list, char *dir ) {
    DIR *modules_dir = NULL;
    struct dirent *ent = NULL;    
    size_t count = 0;
    char **modules_list = NULL;
    char *error = NULL;
    void *mem_module = NULL;
    void *module_handle = NULL;
    unsigned int allocated_mem = 0;
    
    errno = 0;
    
    if( !dir ) {
        return -1;
    }
    
    modules_dir = opendir( dir );
    
    if( !modules_dir ) {
        fprintf( stderr, "%s: %s\n", dir, strerror(errno));
        return -1;
    }
    
    while( ( ent = readdir( modules_dir ) ) ) {
        if( strncmp( ent->d_name, ".", 1 ) == 0 || 
            ( (strstr( ent->d_name, ".so" ) == NULL) && (strstr( ent->d_name, ".dll" ) == NULL) )
            ) {
            continue;
        }
    
        size_t mem_len = ( strlen( ent->d_name ) + strlen( dir ) ) * sizeof( char )  + 2;
        char *module_path = malloc( mem_len );
        memset(module_path, 0,  mem_len);
    
        if( !module_path ) {
            fprintf( stderr, "%s\n", strerror(errno) );
            return -1;
        }
    
        strncat( module_path, dir, strlen( dir ) * sizeof( char ) );
        strncat( module_path, "/", 1 );
        strncat( module_path, ent->d_name, strlen( ent->d_name ) * sizeof( char ) );
    
        module_handle = dlopen ( module_path, RTLD_LAZY );
    
        if( !module_handle ) {
            fprintf( stderr, "Could not load module: '%s'\n", dlerror());
            free( module_path );
            continue;
        }
        
        dlerror();
        runSuite= dlsym( module_handle, "runSuite" );
        error = dlerror();
        if( error ) {
            fprintf( stderr, "Could not load module: %s\n", error);
            dlclose( module_handle );
            free( module_path );
            continue;
        }
    
        mem_module = realloc( modules_list, allocated_mem + strlen(module_path));
        allocated_mem += strlen(module_path);
    
        if( !mem_module ) {
            fprintf( stderr, "%s\n",  strerror(errno));
            free( module_path );
            dlclose( module_handle );
            return -1;
        } 
    
        modules_list = mem_module;
        modules_list[ count ] = module_path;
        count++;
        dlclose( module_handle ); 
    }
    
    closedir( modules_dir );
    qsort(modules_list, count, sizeof(char *), (int (*)(const void *, const void *))modules_alphasort);
    *m_list = modules_list;

    return count;
}

int main() {
    char *modules_dir = NULL;
    char *env_modules_dir = NULL;
    struct stat dir_info;
    size_t modules_total = 0;
    char **modules = NULL;
    size_t i = 0;
    void *module_handle = NULL;

    env_modules_dir = getenv( "TEST_SUITES_DIR" );
    modules_dir = ( env_modules_dir ) ? env_modules_dir : "./suites";
    
    if( stat( modules_dir, &dir_info ) < 0 ) {
       fprintf( stderr, "%s: %s\n", modules_dir, strerror(errno));
       return 1;
    }
    
    if( !S_ISDIR( dir_info.st_mode ) ) {
        fprintf( stderr, "'%s' is not a directory\n", modules_dir);
        return 1;
    } 
    
    if( access( modules_dir, R_OK | X_OK ) != 0 ) {
        fprintf( stderr, "Directory '%s' is not accessible\n", modules_dir );
        return 1;
    } 
    
    modules_total = searchModulesInDir( &modules, modules_dir);
    if(modules_total <= 0) {
        fprintf( stderr, "No test suites\n");
        return 0;
    }

    CUnitInitialize();
    
    for( i = 0; i < modules_total; i++ ) {
        module_handle = dlopen ( modules[i], RTLD_LAZY );	
        if( !module_handle ) {
            fprintf( stderr, "Module '%s'\n", dlerror());
            continue;
        }

        runSuite = dlsym( module_handle, "runSuite" );
        runSuite();
    }
            
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CUnitUInitialize();
    return CU_get_error();
}
