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
//  LOG_{FLOW,INFO,ERROR}( seriousness, format string, parameters... );
//  LOG_{FLOW,INFO,ERROR}0( seriousness, string );
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
#define DEBUG_WAIT_FLOW thread_snooze( DEBUG_WAIT_ON_MSG );
#define DEBUG_WAIT_INFO thread_snooze( DEBUG_WAIT_ON_MSG );
#else
#define DEBUG_WAIT
#define DEBUG_WAIT_FLOW
#define DEBUG_WAIT_INFO
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

#if 1

#define DEBUG_ERROR_COLOR "\x1b[31m"
#define DEBUG_INFO_COLOR "\x1b[33m"
#define DEBUG_FLOW_COLOR "\x1b[32m"
#define DEBUG_NORM_COLOR "\x1b[37m"

#define __GEN_DEBUG_0( __printf, __n, __N, seriousness, format) \
    do { if( seriousness <= debug_level_##__n && seriousness <= DEBUG_MAX_LEVEL_##__N ) { \
    __printf( DEBUG_##__N##_COLOR "%s/%s%s" format DEBUG_NORM_COLOR "\n", FUNC_NAME ); DEBUG_WAIT_##__N \
    }} while( 0 )


#define __GEN_DEBUG_1( __printf, __n, __N,  seriousness, format, param...) \
    do { if( seriousness <= debug_level_##__n && seriousness <= DEBUG_MAX_LEVEL_##__N ) { \
    __printf( DEBUG_##__N##_COLOR "%s/%s%s" format DEBUG_NORM_COLOR "\n", FUNC_NAME, param ); DEBUG_WAIT_##__N \
    }} while( 0 )


#define SHOW_FLOW(seriousness, format, param...)  __GEN_DEBUG_1( printf, flow, FLOW, seriousness, format, param)
#define SHOW_FLOW0(seriousness, format)           __GEN_DEBUG_0( printf, flow, FLOW, seriousness, format)
#define SHOW_INFO(seriousness, format, param...)  __GEN_DEBUG_1( printf, info, INFO, seriousness, format, param)
#define SHOW_INFO0(seriousness, format)           __GEN_DEBUG_0( printf, info, INFO, seriousness, format)
#define SHOW_ERROR(seriousness, format, param...) __GEN_DEBUG_1( printf, error, ERROR, seriousness, format, param)
#define SHOW_ERROR0(seriousness, format)          __GEN_DEBUG_0( printf, error, ERROR, seriousness, format)


// Print to log file only, doubles in kernel/debug.h 
void lprintf(char const *format, ...);


#define __GEN_LOG( __n, __N,  seriousness, format, param...) \
    do { if( seriousness <= debug_level_##__n && seriousness <= DEBUG_MAX_LEVEL_##__N ) { \
    lprintf( "%s/%s%s" format "\n", FUNC_NAME, param ); \
    }} while( 0 )


#define LOG_FLOW(seriousness, format, param...)  __GEN_LOG( flow, FLOW, seriousness, format, param)
#define LOG_FLOW0(seriousness, format)           __GEN_LOG( flow, FLOW, seriousness, format, 0)
#define LOG_INFO(seriousness, format, param...)  __GEN_LOG( info, INFO, seriousness, format, param)
#define LOG_INFO0(seriousness, format)           __GEN_LOG( info, INFO, seriousness, format, 0)
#define LOG_ERROR(seriousness, format, param...) __GEN_LOG( error, ERROR, seriousness, format, param)
#define LOG_ERROR0(seriousness, format)          __GEN_LOG( error, ERROR, seriousness, format, 0)



#else

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


#endif
