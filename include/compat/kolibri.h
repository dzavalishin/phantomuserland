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
#include <kernel/net_timer.h>
#include <kernel/pool.h>
#include <video/window.h>
#include <video/color.h>

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
    char                ident[8];
    u_int32_t           version;
    u_int32_t           start;
    u_int32_t           code_end;
    u_int32_t           data_end;
    u_int32_t           stack_end;
    u_int32_t           params;
    u_int32_t           exe_name; 	// icon??
};

struct kolibri_pkck_hdr
{
    char                ident[4];
    u_int32_t           unpacked_size;
    u_int32_t           flags;
};

#define KOLIBRI_CMD_LINE_MAX 256
#define KOLIBRI_CMD_PATH_MAX 256

#define MAX_COLIBRI_EXE_SIZE (16*1024*1024)

typedef struct kolibri_exe_hdr kolibri_exe_hdr_t;

static inline errno_t is_not_kolibri_exe( kolibri_exe_hdr_t *exe )
{
    if( 0 == strncmp( "KPCK", exe->ident, 4 ) )
        return 0;

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
    u_int32_t           event_bits; // Kolibri event bits

    window_handle_t     win;
    net_timer_event 	win_update_timer;
    u_int32_t           win_update_prevent;
    u_int32_t           win_alpha_scale;
    u_int8_t            *win_user_alpha;

    pool_t              *buttons;
    u_int32_t           pressed_button_id;
    u_int32_t           pressed_button_mouseb; // mouse buttons

    u_int32_t           key_input_scancodes;

    wtty_t *            keys;

    int                 (*defaultEventProcess)( struct drv_video_window *w, struct ui_event *e );


    void *              ipc_buf_addr;
    size_t              ipc_buf_size;

    struct ui_event 	e;
    int                 have_e;
};

typedef struct kolibri_process_state kolibri_state_t;

struct kolibri_button
{
    int         id;
    rect_t      r;
    color_t     color;

    int         flag_nopaint;
    int         flag_noborder;

    u_int32_t   mouse_in_bits;

    int         npixels;
    rgba_t      *pixels;
};

#define BCD_BYTE(___i) (((___i % 10) | ((___i / 10) << 4)) & 0xFF )


struct kolibri_kernel_version
{
    char 	a, b, c, d; // x.y.z.w
    char 	x; // unused
    u_int32_t 	svn_rev;
} __attribute__((__packed__));

struct kolibri_thread_info
{
    u_int32_t           cpu_usage; // units?
    u_int16_t           win_z_order;
    u_int16_t           ecx_win_slot; // GOD HELP US... read http://wiki.kolibrios.org/wiki/SysFn09/ru
    u_int16_t           reserved1;

    char                name[11];
    char                reserved2;

    u_int32_t           mem_addr;
    u_int32_t           mem_size; // -1 to real == descriptor limit?!

    u_int32_t           tid;

    u_int32_t           x;
    u_int32_t           y;
    u_int32_t           xsize;
    u_int32_t           ysize;

    u_int16_t           state; // 0 - run, 1 - blocked, 2 - blokc + wait 4 event??, 3 - zombie, 4 - exception zombie, 5 - wait 4 event, 9 - slot is empty
    u_int16_t           reserved3;

    // Client area
    u_int32_t           cx;
    u_int32_t           cy;
    u_int32_t           cxsize;
    u_int32_t           cysize;

    u_int8_t            win_state; // & 0x1 - max, & 0x2 - min to tray, & 0x4 - rolled

    u_int32_t           event_mask;
} __attribute__((__packed__));


struct kolibri_color_defaults
{
    rgba_t              border_color;
    rgba_t              header_color;

    rgba_t              button_color;
    rgba_t              button_text_color;
    rgba_t              title_text_color;

    rgba_t              work_color;
    rgba_t              work_button_color;
    rgba_t              work_button_text_color;

    rgba_t              work_text_color;
    rgba_t              work_graph_color;

} __attribute__((__packed__));


struct kolibri_thread_start_parm
{
    addr_t eip;
    addr_t esp;
};


struct kolibri_ipc_msg
{
    u_int32_t           tid;
    u_int32_t           len;
    char                data[];
};

typedef struct kolibri_ipc_msg kolibri_ipc_msg_t;


struct kolibri_ipc_buf
{
    u_int32_t           busy;
    u_int32_t           used;
    kolibri_ipc_msg_t   msg[];
};

typedef struct kolibri_ipc_buf kolibri_ipc_buf_t;

// -----------------------------------------------------------------------
// original defs
// -----------------------------------------------------------------------

#define FONT0          0x00000000
#define FONT1          0x10000000

#define BT_NORMAL      0x00000000
#define BT_NOFRAME     0x20000000
#define BT_HIDE        0x40000000
#define BT_DEL         0x80000000

// This is bit number + 1
#define EV_REDRAW      1
#define EV_KEY         2
#define EV_BUTTON      3
#define EV_MOUSE       6
#define EV_IPC         7
#define EV_NET         8
#define EV_DEBUG       9

#define KOLIBRI_SET_EVENT_BIT(bits,nb) ((bits) |= (1<<((nb)-1)))
#define KOLIBRI_RESET_EVENT_BIT(bits,nb) ((bits) &= ~(1<<((nb)-1)))

#define KOLIBRI_HAS_EVENT_BIT(bits,nb) ((bits) & (1<<((nb)-1)))

#define REL_SCREEN     0
#define REL_WINDOW     1

//#define FILE_NOT_FOUND 5
//#define FILE_EOF       6


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


