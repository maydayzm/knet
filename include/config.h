/*
 * Copyright (c) 2014-2015, dennis wang
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#if defined(WIN32)
    #if defined(_WIN32_WINNT)
        #if _WIN32_WINNT < 0x0502
            #define _WIN32_WINNT 0x0502
        #endif /* _WIN32_WINNT < 0x0502 */ 
    #else
        #define _WIN32_WINNT 0x0502 /* TryEnterCriticalSection */
    #endif /* defined(_WIN32_WINNT) */
    #if defined(FD_SETSIZE)
        #undef FD_SETSIZE
        #define FD_SETSIZE 1024
    #else
        #define FD_SETSIZE 1024
    #endif /* defined(FD_SETSIZE) */
    #include <winsock2.h>
    #include <mswsock.h>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
    #if defined(_MSC_VER )
        #pragma comment(lib,"wsock32.lib")
    #endif /* defined(_MSC_VER) */
    typedef SOCKET socket_t;
    typedef int socket_len_t;
    typedef uintptr_t thread_id_t;
    typedef DWORD sys_error_t;
    typedef volatile LONG atomic_counter_t;
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
    typedef signed short int16_t;
    typedef unsigned int uint32_t;
    typedef signed int int32_t;
    typedef unsigned long long uint64_t ;
    typedef signed long long int64_t;
    #ifndef PATH_MAX
        #define PATH_MAX MAX_PATH
    #endif /* PATH_MAX */
#else
    #include <stdint.h>
    #include <errno.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/epoll.h>
    #define socket_len_t socklen_t
    #define thread_id_t pthread_t
    #define socket_t int
    #define sys_error_t int
    #define atomic_counter_t volatile int
#endif /* defined(WIN32) */

#define float32_t float
#define float64_t double

#ifndef INT_MAX
/* from stdint.h */
#define INT_MAX  2147483647 /* maximum (signed) int value */
#endif /* INT_MAX */

#define create(type)                  (type*)malloc(sizeof(type))
#define create_raw(size)              (char*)malloc(size)
#define create_type(type, size)       (type*)malloc(size)
#define rcreate_raw(ptr, size)        (char*)realloc(ptr, size)
#define rcreate_type(type, ptr, size) (type*)realloc(ptr, size)
#define destroy(ptr)                  do { if (ptr) { free(ptr); } } while(0);

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct _loop_t kloop_t;
typedef struct _channel_t kchannel_t;
typedef struct _channel_ref_t kchannel_ref_t;
typedef struct _address_t kaddress_t;
typedef struct _lock_t klock_t;
typedef struct _loop_balancer_t kloop_balancer_t;
typedef struct _thread_runner_t kthread_runner_t;
typedef struct _stream_t kstream_t;
typedef struct _dlist_t kdlist_t;
typedef struct _dlist_node_t kdlist_node_t;
typedef struct _ringbuffer_t kringbuffer_t;
typedef struct _buffer_t kbuffer_t;
typedef struct _broadcast_t kbroadcast_t;
typedef struct _ktimer_loop_t ktimer_loop_t;
typedef struct _ktimer_t ktimer_t;
typedef struct _logger_t klogger_t;
typedef struct _krpc_t krpc_t;
typedef struct _krpc_number_t krpc_number_t;
typedef struct _krpc_string_t krpc_string_t;
typedef struct _krpc_vector_t krpc_vector_t;
typedef struct _krpc_object_t krpc_object_t;
typedef struct _krpc_map_t krpc_map_t;
typedef struct _krpc_value_t krpc_value_t;
typedef struct _hash_t khash_t;
typedef struct _hash_value_t khash_value_t;
typedef struct _framework_t kframework_t;
typedef struct _framework_acceptor_config_t kframework_acceptor_config_t;
typedef struct _framework_connector_config_t kframework_connector_config_t;
typedef struct _framework_config_t kframework_config_t;
typedef struct _framework_raiser_t kframework_raiser_t;
typedef struct _framework_worker_t kframework_worker_t;
typedef struct _framework_timer_config_t kframework_timer_config_t;
typedef struct _loop_profile_t kloop_profile_t;
typedef struct _trie_t ktrie_t;
typedef struct _ip_filter_t kip_filter_t;
typedef struct _vrouter_t kvrouter_t;
typedef struct _router_path_t krouter_path_t;

/* �ܵ���Ͷ���¼� */
typedef enum _channel_event_e {
    channel_event_recv = 1,
    channel_event_send = 2,
} knet_channel_event_e;

/*! �ܵ�״̬ */
typedef enum _channel_state_e {
    channel_state_connect = 1, /*! �����������ӣ�����δ��� */
    channel_state_accept = 2,  /*! ���� */
    channel_state_close = 4,   /*! �ܵ��ѹر� */
    channel_state_active = 8,  /*! �ܵ��Ѽ�������շ����� */
    channel_state_init = 16,   /*! �ܵ��ѽ�������δ���� */
} knet_channel_state_e;

/*! ��ʱ������ */
typedef enum _ktimer_type_e {
    ktimer_type_once   = 1, /*! ����һ�� */
    ktimer_type_period = 2, /*! ���� */
    ktimer_type_times  = 3, /*! ������� */
} ktimer_type_e;

/*! ���ؾ������� */
typedef enum _loop_balance_option_e {
    loop_balancer_in  = 1, /*! ��������kloop_t�Ĺܵ��ڵ�ǰkloop_t���� */
    loop_balancer_out = 2, /*! ������ǰkloop_t�Ĺܵ�������kloop_t�ڸ��� */
} knet_loop_balance_option_e;

/* ������ */
typedef enum _error_e {
    error_ok = 0,
    error_fail,
    error_invalid_parameters,
    error_must_be_shared_channel_ref,
    error_invalid_channel,
    error_invalid_broadcast,
    error_no_memory,
    error_hash_not_found,
    error_recv_fail,
    error_send_fail,
    error_send_patial,
    error_recv_buffer_full,
    error_recv_nothing,
    error_connect_fail,
    error_connect_in_progress,
    error_channel_not_connect,
    error_accept_in_progress,
    error_bind_fail,
    error_listen_fail,
    error_ref_nonzero,
    error_loop_fail,
    error_loop_attached,
    error_loop_not_found,
    error_loop_impl_init_fail,
    error_thread_start_fail,
    error_already_close,
    error_impl_add_channel_ref_fail,
    error_getpeername,
    error_getsockname,
    error_not_correct_domain,
    error_multiple_start,
    error_not_connected,
    error_logger_write,
    error_set_tls_fail,
    error_rpc_dup_id,
    error_rpc_unknown_id,
    error_rpc_unknown_type,
    error_rpc_cb_fail,
    error_rpc_cb_fail_close,
    error_rpc_cb_close,
    error_rpc_next,
    error_rpc_not_enough_bytes,
    error_rpc_vector_out_of_bound,
    error_rpc_marshal_fail,
    error_rpc_unmarshal_fail,
    error_rpc_map_error_key_or_value,
    error_recvbuffer_not_enough,
    error_recvbuffer_locked,
    error_stream_enable,
    error_stream_disable,
    error_stream_flush,
    error_stream_buffer_overflow,
    error_trie_not_found,
    error_trie_key_exist,
    error_trie_for_each_fail,
    error_ip_filter_open_fail,
    error_router_wire_not_found,
    error_router_wire_exist,
} knet_error_e;

/*! �ܵ��ص��¼� */
typedef enum _channel_cb_event_e {
    channel_cb_event_connect = 1,          /*! ������� */
    channel_cb_event_accept = 2,           /*! �ܵ������������������� */ 
    channel_cb_event_recv = 4,             /*! �ܵ������ݿ��Զ� */
    channel_cb_event_send = 8,             /*! �ܵ��������ֽڣ����� */
    channel_cb_event_close = 16,           /*! �ܵ��ر� */
    channel_cb_event_timeout = 32,         /*! �ܵ������� */
    channel_cb_event_connect_timeout = 64, /*! �����������ӣ������ӳ�ʱ */
} knet_channel_cb_event_e;

/* ��־�ȼ� */
typedef enum _logger_level_e {
    logger_level_verbose = 1, /* verbose - ������� */
    logger_level_information, /* information - ��ʾ��Ϣ */
    logger_level_warning,     /* warning - ���� */ 
    logger_level_error,       /* error - ���� */
    logger_level_fatal,       /* fatal - �������� */
} knet_logger_level_e;

/* ��־ģʽ */
typedef enum _logger_mode_e {
    logger_mode_file = 1,     /* ������־�ļ� */
    logger_mode_console = 2,  /* ��ӡ��stderr */
    logger_mode_flush = 4,    /* ÿ��д��־ͬʱ��ջ��� */
    logger_mode_override = 8, /* �����Ѵ��ڵ���־�ļ� */
} knet_logger_mode_e;

/*! RPC������ */
typedef enum _rpc_error_e {
    rpc_ok = 0,      /*! �ɹ� */
    rpc_close,       /*! ���Դ��󣬹ر� */
    rpc_error,       /*! ���󣬵����ر� */
    rpc_error_close, /*! �����ҹر� */
} knet_rpc_error_e;

/*! RPC���� */
typedef enum _krpc_type_e {
    krpc_type_i8     = 1,    /*! �з���8λ */
    krpc_type_ui8    = 2,    /*! �޷���8λ */
    krpc_type_i16    = 4,    /*! �з���16λ */
    krpc_type_ui16   = 8,    /*! �޷���16λ */
    krpc_type_i32    = 16,   /*! �з���32λ */
    krpc_type_ui32   = 32,   /*! �޷���32λ */
    krpc_type_i64    = 64,   /*! �з���64λ */
    krpc_type_ui64   = 128,  /*! �޷���64λ */
    krpc_type_f32    = 256,  /*! 32λ���� */
    krpc_type_f64    = 512,  /*! 64λ���� */
    krpc_type_number = 1024, /*! ���� */
    krpc_type_string = 2048, /*! �ַ��� */
    krpc_type_vector = 4096, /*! ���� */
    krpc_type_map    = 8192, /*! �� */
} knet_rpc_type_e;

/*! �̺߳��� */
typedef void (*knet_thread_func_t)(kthread_runner_t*);
/*! �ܵ��¼��ص����� */
typedef void (*knet_channel_ref_cb_t)(kchannel_ref_t*, knet_channel_cb_event_e);
/*! ��ʱ���ص����� */
typedef void (*ktimer_cb_t)(ktimer_t*, void*);
/*! RPC�ص����� */
typedef int (*krpc_cb_t)(krpc_object_t*);
/*! RPC���ܻص�����, ���� ���� ���ܺ󳤶�, 0 ʧ�� */
typedef uint16_t (*krpc_encrypt_t)(void*, uint16_t, void*, uint16_t);
/*! RPC���ܻص�����, ���� ���� ���ܺ󳤶�, 0 ʧ�� */
typedef uint16_t (*krpc_decrypt_t)(void*, uint16_t, void*, uint16_t);
/*! ��ϣ��Ԫ�����ٺ��� */
typedef void (*knet_hash_dtor_t)(void*);
/*! trieԪ�����ٺ��� */
typedef void (*knet_trie_dtor_t)(void*);
/*! trie�������� */
typedef int (*knet_trie_for_each_func_t)(const char*, void*);

/* ������Ҫ�� ������ͬѡȡ�� */
#if defined(WIN32)
    #define LOOP_IOCP 1    /* IOCP */
    #define LOOP_SELECT 0  /* select */
#else
    #define LOOP_EPOLL 1   /* epoll */
    #define LOOP_SELECT 0  /* select */
#endif /* defined(WIN32) */

#if defined(DEBUG) || defined(_DEBUG)
    #define LOGGER_ON 1 /* ���԰汾������־ */
#else
    #define LOGGER_ON 0 /* ���а�ر���־ */
#endif /* defined(DEBUG) || defined(_DEBUG) */

#define LOGGER_MODE (logger_mode_file | logger_mode_console | logger_mode_flush | logger_mode_override) /* ��־ģʽ */
#define LOGGER_LEVEL logger_level_verbose /* ��־�ȼ� */

#include "logger.h"

#if defined(DEBUG) || defined(_DEBUG)
    #define verify(expr) assert(expr)
#elif defined(NDEBUG)
    #define verify(expr) \
        if (!(expr)) { \
            log_fatal("crash point abort, file:(%s:%d): cause:(%s)", __FILE__, __LINE__, #expr); \
            abort(); \
        }
#else
    #define verify(expr) \
        if (!(expr)) { \
            log_fatal("crash point abort, file:(%s:%d): cause:(%s)", __FILE__, __LINE__, #expr); \
            abort(); \
        }
#endif /* defined(DEBUG) || defined(_DEBUG) */

#define KRPC_MEMBER_SET(type, key)\
	void	krpc_member_set(krpc_object_t* t, type node){\
		krpc_number_set_##key(t, node);\
	}

#define KRPC_MEMBER_GET(type, key)\
	void	krpc_member_get(krpc_object_t* t, type& node){\
		node = krpc_number_get_##key(t);\
	}

#define KRPC_MARSHAL_COMM( type )\
	krpc_object_t*	marshal(type node){\
		krpc_object_t* pNode = krpc_object_create();\
		krpc_member_set(pNode, node);\
		return pNode;\
	}

#define KRPC_UNMARSHAL_COMM( type )\
	bool unmarshal(krpc_object_t* v, type& node){\
		krpc_member_get(v, node);\
	return true;\
}

#endif /* CONFIG_H */