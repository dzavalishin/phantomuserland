/*
** Copyright 2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __DEBUG_EXT_H__
#define __DEBUG_EXT_H__

// this is a dprintf wrapper
//
// there are three kinds of messages:
//  flow: used to trace execution
//  info: tells things that are important but not an error
//  error: used if something has gone wrong
//
// common usage is
//  SHOW_{FLOW,INFO,ERROR}( seriousness, format string, parameters... );
//  SHOW_{FLOW,INFO,ERROR}0( seriousness, string );
//
// with
//  seriousness: the smaller the more serious (0..3)
//  format string, parameters: normal printf stuff
//
// to specify the module that created the message you have
// to define a string called
//  DEBUG_MSG_PREFIX
// you dynamically speficify the maximum seriousness level by defining
// the following variables/macros
//  debug_level_flow
//  debug_level_info
//  debug_level_error
//
// you _can_ statically specify the maximum seriuosness level by defining
//  DEBUG_MAX_LEVEL_FLOW
//  DEBUG_MAX_LEVEL_INFO
//  DEBUG_MAX_LEVEL_ERRROR
//
// you _can_ ask to delay execution after each printed message
// by defining the duration (in ms) via
//  DEBUG_WAIT_ON_MSG (for flow and info)
//  DEBUG_WAIT_ON_ERROR (for error)

#include "console.h"

#ifdef DEBUG_WAIT_ON_MSG
#define DEBUG_WAIT thread_snooze( DEBUG_WAIT_ON_MSG );
#else
#define DEBUG_WAIT
#endif

#ifdef DEBUG_WAIT_ON_ERROR
#define DEBUG_WAIT_ERROR thread_snooze( DEBUG_WAIT_ON_ERROR );
#else
#define DEBUG_WAIT_ERROR
#endif

extern int debug_max_level_flow;
extern int debug_max_level_info;
extern int debug_max_level_error;


#ifndef DEBUG_MAX_LEVEL_FLOW
#define DEBUG_MAX_LEVEL_FLOW debug_max_level_flow
#endif

#ifndef DEBUG_MAX_LEVEL_INFO
#define DEBUG_MAX_LEVEL_INFO debug_max_level_info
#endif

#ifndef DEBUG_MAX_LEVEL_ERROR
#define DEBUG_MAX_LEVEL_ERROR debug_max_level_error
#endif

#ifndef DEBUG_MSG_PREFIX
#error you need to define DEBUG_MSG_PREFIX with the module name
#endif

#define FUNC_NAME DEBUG_MSG_PREFIX, __FUNCTION__, ": "

#define SHOW_FLOW(seriousness, format, param...) \
    do { if( seriousness <= debug_level_flow && seriousness <= DEBUG_MAX_LEVEL_FLOW ) { \
    console_set_message_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME, param ); DEBUG_WAIT \
    console_set_normal_color(); \
    }} while( 0 )

#define SHOW_FLOW0(seriousness, format) \
    do { if( seriousness <= debug_level_flow && seriousness <= DEBUG_MAX_LEVEL_FLOW ) { \
    console_set_message_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME); DEBUG_WAIT \
    console_set_normal_color(); \
    }} while( 0 )

#define SHOW_INFO(seriousness, format, param...) \
    do { if( seriousness <= debug_level_info && seriousness <= DEBUG_MAX_LEVEL_INFO ) { \
    console_set_warning_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME, param ); DEBUG_WAIT \
    console_set_normal_color(); \
    }} while( 0 )

#define SHOW_INFO0(seriousness, format) \
    do { if( seriousness <= debug_level_info && seriousness <= DEBUG_MAX_LEVEL_INFO ) { \
    console_set_warning_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME); DEBUG_WAIT \
    console_set_normal_color(); \
    }} while( 0 )

#define SHOW_ERROR(seriousness, format, param...) \
    do { if( seriousness <= debug_level_error && seriousness <= DEBUG_MAX_LEVEL_ERROR ) { \
    console_set_error_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME, param ); DEBUG_WAIT_ERROR \
    console_set_normal_color(); \
    }} while( 0 )

#define SHOW_ERROR0(seriousness, format) \
    do { if( seriousness <= debug_level_error && seriousness <= DEBUG_MAX_LEVEL_ERROR ) { \
    console_set_error_color(); \
    printf( "%s/%s%s"format"\n", FUNC_NAME); DEBUG_WAIT_ERROR \
    console_set_normal_color(); \
    }} while( 0 )

#endif
