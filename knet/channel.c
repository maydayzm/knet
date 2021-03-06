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

#include "channel.h"
#include "buffer.h"
#include "list.h"
#include "ringbuffer.h"
#include "loop.h"
#include "misc.h"
#include "logger.h"


struct _channel_t {
    kdlist_t*      send_buffer_list;  /* 发送链表, 发送失败的数据会法如这个链表等待下次发送 */
    uint32_t       max_send_list_len; /* 发送链表最大长度 */
    kringbuffer_t* recv_ringbuffer;   /* 读环形缓冲区, 通过socket读取函数读到的数据会放在这个缓冲区内 */
    socket_t       socket_fd;         /* 套接字 */
    uint64_t       uuid;              /* 管道UUID */
};

kchannel_t* knet_channel_create(uint32_t max_send_list_len, uint32_t recv_ring_len) {
    socket_t socket_fd = socket_create();
    verify(socket_fd > 0);
    if (socket_fd <= 0) {
        return 0;
    }
    return knet_channel_create_exist_socket_fd(socket_fd, max_send_list_len, recv_ring_len);
}

kchannel_t* knet_channel_create_exist_socket_fd(socket_t socket_fd, uint32_t max_send_list_len, uint32_t recv_ring_len) {
    kchannel_t* channel = create(kchannel_t);
    verify(channel);
    memset(channel, 0, sizeof(kchannel_t));
    channel->uuid = uuid_create();
    channel->send_buffer_list = dlist_create();
    verify(channel->send_buffer_list);
    channel->recv_ringbuffer = ringbuffer_create(recv_ring_len);
    verify(channel->recv_ringbuffer);
    channel->max_send_list_len = max_send_list_len;
    channel->socket_fd = socket_fd;
    /* 设置为非阻塞 */
    socket_set_non_blocking_on(channel->socket_fd);
    /* 关闭延迟发送 */
    socket_set_nagle_off(channel->socket_fd);
    /* 关闭TIME_WAIT */
    socket_set_linger_off(channel->socket_fd);
    /* 关闭keep alive */
    socket_set_keepalive_off(channel->socket_fd);
    return channel;
}

void knet_channel_destroy(kchannel_t* channel) {
    kdlist_node_t* node        = 0;
    kdlist_node_t* temp        = 0;
    kbuffer_t*     send_buffer = 0;
    verify(channel);
    /* 销毁未发送的数据 */
    if (channel->send_buffer_list) {
        dlist_for_each_safe(channel->send_buffer_list, node, temp) {
            send_buffer = (kbuffer_t*)dlist_node_get_data(node);
            knet_buffer_destroy(send_buffer);
        }
        dlist_destroy(channel->send_buffer_list);
    }
    /* 销毁接收缓冲区 */
    if (channel->recv_ringbuffer) {
        ringbuffer_destroy(channel->recv_ringbuffer);
    }
    destroy(channel);
}

int knet_channel_connect(kchannel_t* channel, const char* ip, int port) {
    verify(channel);
    verify(ip);
    return socket_connect(channel->socket_fd, ip, port);
}

int knet_channel_accept(kchannel_t* channel, const char* ip, int port, int backlog) {
    verify(channel);
    verify(port);
    if (!backlog) {
        backlog = 50;
    }
    if (!ip) {
        ip = "0.0.0.0";
    }
    /* 设置为监听状态 */
    return socket_bind_and_listen(channel->socket_fd, ip, port, backlog);
}

int knet_channel_send_buffer(kchannel_t* channel, kbuffer_t* send_buffer) {
    verify(channel);
    verify(send_buffer);
    verify(channel->send_buffer_list);
    /* 始终无法发送 */
    if (knet_channel_send_list_reach_max(channel)) {
        knet_buffer_destroy(send_buffer);
        return error_send_fail;
    }
    /* 将发送缓冲区加到链表尾部 */
    dlist_add_tail_node(channel->send_buffer_list, send_buffer);
    /* 让调用者重新设置写事件 */
    return error_send_patial;
}

int knet_channel_send(kchannel_t* channel, const char* data, int size) {
    int        bytes       = 0;
    kbuffer_t* send_buffer = 0;
    verify(channel);
    verify(data);
    verify(size);
    verify(channel->send_buffer_list);
    /* 始终无法发送 */
    if (knet_channel_send_list_reach_max(channel)) {
        return error_send_fail;
    }
    if (dlist_empty(channel->send_buffer_list)) {
        /* 尝试直接发送 */
        bytes = socket_send(channel->socket_fd, data, size);
    }
    if (bytes < 0) {
        return error_send_fail;
    }
    /* 直接发送失败，或者没有发送完毕的字节放入发送链表等待下次发送 */
    if (size > bytes) {
        send_buffer = knet_buffer_create(size - bytes);
        verify(send_buffer);
        knet_buffer_put(send_buffer, data + bytes, size - bytes);
        dlist_add_tail_node(channel->send_buffer_list, send_buffer);
        /* 需要稍后发送 */
        return error_send_patial;
    }
    return error_ok;
}

int knet_channel_update_send(kchannel_t* channel) {
    kdlist_node_t* node        = 0;
    kdlist_node_t* temp        = 0;
    kbuffer_t*     send_buffer = 0;
    int           bytes       = 0;
    verify(channel);
    verify(channel->send_buffer_list);
    /* 发送链表内所有数据 */
    dlist_for_each_safe(channel->send_buffer_list, node, temp) {
        send_buffer = (kbuffer_t*)dlist_node_get_data(node);
        bytes = socket_send(channel->socket_fd, knet_buffer_get_ptr(send_buffer), knet_buffer_get_length(send_buffer));
        if (bytes < 0) {
            return error_send_fail;
        }
        if (knet_buffer_get_length(send_buffer) > (uint32_t)bytes) {
            /* 本次未发送完毕，调整buffer长度，等待下次发送 */
            knet_buffer_adjust(send_buffer, bytes);
            /* 部分发送 */
            return error_send_patial;
        } else {
            /* 销毁已发送节点 */
            knet_buffer_destroy(send_buffer);
            dlist_delete(channel->send_buffer_list, node);
        }
    }
    /* 全部发送 */
    return error_ok;
}

int knet_channel_update_recv(kchannel_t* channel) {
    int      bytes      = 0;
    int      recv_bytes = 0;
    uint32_t size       = 0;
    char*    ptr        = 0;
    verify(channel);
    verify(channel->recv_ringbuffer);
    if (ringbuffer_full(channel->recv_ringbuffer)) {
        /* 读缓冲区满，关闭, 防攻击, 可根据需求调整大小 */
        return error_recv_buffer_full;
    }
    for (; (size = ringbuffer_write_lock_size(channel->recv_ringbuffer));) {
        ptr = ringbuffer_write_lock_ptr(channel->recv_ringbuffer);
        bytes = socket_recv(channel->socket_fd, ptr, size);
        if (bytes < 0) {
            /* 错误，关闭 */
            return error_recv_fail;
        } else if (bytes == 0) {
            /* 未接收到, 下次继续接收 */
            ringbuffer_write_commit(channel->recv_ringbuffer, 0);
            return error_ok;
        } else {
            recv_bytes += bytes;
            /* 接收到 */
            ringbuffer_write_commit(channel->recv_ringbuffer, (uint32_t)bytes);
        }
    }
    if (!recv_bytes) {
        /* 本次不能完成接收，非关闭类错误 */
        return error_recv_nothing;
    }
    return error_ok;
}

void knet_channel_close(kchannel_t* channel) {
    verify(channel);
    socket_close(channel->socket_fd);
}

socket_t knet_channel_get_socket_fd(kchannel_t* channel) {
    verify(channel);
    return channel->socket_fd;
}

kringbuffer_t* knet_channel_get_ringbuffer(kchannel_t* channel) {
    verify(channel);
    return channel->recv_ringbuffer;
}

uint32_t knet_channel_get_max_send_list_len(kchannel_t* channel) {
    verify(channel);
    return channel->max_send_list_len;
}

uint32_t knet_channel_get_max_recv_buffer_len(kchannel_t* channel) {
    verify(channel);
    return ringbuffer_get_max_size(channel->recv_ringbuffer);
}

uint64_t knet_channel_get_uuid(kchannel_t* channel) {
    verify(channel);
    return channel->uuid;
}

int knet_channel_send_list_reach_max(kchannel_t* channel) {
    verify(channel);
    return (dlist_get_count(channel->send_buffer_list) > (int)channel->max_send_list_len);
}
