/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kolibri emulation.
 *
**/

#ifndef KOLIBRI_H
#define KOLIBRI_H

#include <phantom_types.h>
#include <errno.h>
#include <string.h>
#include <kernel/sem.h>
#include <kernel/pool.h>
#include <video/window.h>

/*
 db 'MENUET01'   ; 1. идентификатор исполняемого файла (8 байт)
 dd 0x01	  ; 2. версия формата заголовка исполняемого файла
 dd START	  ; 3. адрес, на который система передаёт управление
 dd I_END	  ; 4. размер приложения
 dd I_END+0x1000 ; 5. объём необходимой приложению памяти
 dd I_END+0x1000 ; 6. вершина стека в диапазоне памяти, указанном выше
 dd 0x0	  ; 7. указатель на строку с параметрами.
 dd 0x0	  ; 8. указатель на строку, в которую записан путь запуска
*/

struct kolibri_exe_hdr
{
    char        	ident[8];
    u_int32_t           version;
    u_int32_t           start;
    u_int32_t           code_end;
    u_int32_t           data_end;
    u_int32_t           stack_end;
    u_int32_t           params;
    u_int32_t           exe_name; 	// icon??
};

typedef struct kolibri_exe_hdr kolibri_exe_hdr_t;

static inline errno_t is_not_kolibri_exe( kolibri_exe_hdr_t *exe )
{
    if( strncmp( "MENUET01", exe->ident, 8 ) )
        return ENOEXEC;

    // seems to mean nothing
    //if( exe->version != 1 )        return ENOEXEC;

    return 0;
}

struct kolibri_process_state
{
    hal_mutex_t         lock; // general process lock - access to process resources

    hal_sem_t           event; // syscall 23 event - release when set bit in event_state
    u_int32_t           event_mask;
    u_int32_t           event_state; // Kolibri event bits

    window_handle_t     win;

    pool_t              *buttons;
};


struct kolibri_button
{
    int         id;
    rect_t      r;

    int         flag_nopaint;
    int         flag_noborder;

    int         npixels;
    rgba_t      *pixels;
};

#define BCD_BYTE(___i) (((___i % 10) | ((___i / 10) << 4)) & 0xFF )


// -----------------------------------------------------------------------
// original defs
// -----------------------------------------------------------------------

#define FONT0          0x00000000
#define FONT1          0x10000000

#define BT_NORMAL      0x00000000
#define BT_NOFRAME     0x20000000
#define BT_HIDE        0x40000000
#define BT_DEL         0x80000000

#define EV_REDRAW      1
#define EV_KEY         2
#define EV_BUTTON      3

#define REL_SCREEN     0
#define REL_WINDOW     1

#define FILE_NOT_FOUND 5
#define FILE_EOF       6


//typedef unsigned int DWORD;
//typedef unsigned short int WORD;

#define DWORD u_int32_t
#define WORD u_int16_t

typedef struct
{  DWORD pci_cmd;
   DWORD irq;
   DWORD glob_cntrl;
   DWORD glob_sta;
   DWORD codec_io_base;
   DWORD ctrl_io_base;
   DWORD codec_mem_base;
   DWORD ctrl_mem_base;
   DWORD codec_id;
} kolibri_CTRL_INFO;

typedef struct
{   DWORD       cmd;
    DWORD       offset;
    DWORD       r1;
    DWORD       count;
    DWORD       buff;
    char        r2;
    char       *name;
} kolibri_FILEIO;

typedef struct
{   DWORD    attr;
    DWORD    flags;
    DWORD    cr_time;
    DWORD    cr_date;
    DWORD    acc_time;
    DWORD    acc_date;
    DWORD    mod_time;
    DWORD    mod_date;
    DWORD    size;
    DWORD    size_high; 
} kolibri_FILEINFO;

#undef DWORD
#undef WORD 

// Impossible on Phantom?
#define KERR_NO_PARTITION 1
// Not on this FS
#define KERR_IVALID_FUNC 2
// Impossible on Phantom?
#define KERR_UNKNOWN_FS 3

//#define KERR_RESERVED 4

// File not found
#define KERR_NOENT 5

#define KERR_EOF 6

// memory access
#define KERR_FAULT 7

#define KERR_DISK_FULL 8

// Impossible on Phantom?
#define KERR_FS_INSANE 9


#define KERR_ACCESS_DENIED 10

#define KERR_IO 11

// spawn - out of mem
#define KERR_NOMEM 30
// spawn - file is not executable
#define KERR_NOEXEC 31
#define KERR_TOO_MANY_PROC 32


#endif // KOLIBRI_H


