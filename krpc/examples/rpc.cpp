#include <iostream>
#include <sstream>
#include "rpc_sample.h"

using namespace rpc_sample;

namespace rpc_sample {

int my_rpc_func1(my_object_t& my_obj) {
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
    std::cout << "invoke my_rpc_func1!" << std::endl;
    std::stringstream ss;
    my_obj.print(ss);
    std::cout << ss.str();
    return rpc_ok;
}

int my_rpc_func2(std::vector<my_object_t>& my_objs, int8_t my_i8) {
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
    std::cout << "invoke my_rpc_func2!" << std::endl;
    return rpc_ok;
}

int my_rpc_func3(const std::string& my_str, int8_t my_i8) {
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
    std::cout << "invoke my_rpc_func3!" << std::endl;
    std::cout << "my_str=" << my_str << std::endl;
    std::cout << "my_i8=" << (int)my_i8 << std::endl;
    return rpc_ok;
}

}

/* 客户端 - 连接器回调 */
void connector_cb(kchannel_ref_t* channel, knet_channel_cb_event_e e) {
    kstream_t* stream = knet_channel_ref_get_stream(channel);
    if (e & channel_cb_event_connect) { /* 连接成功 */
        /* 调用RPC函数 */
        my_object_other_t my_other_obj;
        my_other_obj.string_string_table["name"] = "i'm a string";

        my_object_t my_obj;
        my_obj.ni8 = 1;
        my_obj.ni16 = 2;
        my_obj.ni32 = 3;
        my_obj.ni64 = 4;
        my_obj.nui8 = 5;
        my_obj.nui16 = 6;
        my_obj.nui32 = 7;
        my_obj.nui64 = 8;
        my_obj.nf32 = 9.0f;
        my_obj.nf64 = 10.0f;
        my_obj.str = "hello world!";
        my_obj.int_string_table[1] = "i'm a string";
        my_obj.int_object_table[1] = my_other_obj;
        std::vector<my_object_t> my_obj_vec;
        my_obj_vec.push_back(my_obj);
        my_obj_vec.push_back(my_obj);
        rpc_sample_ptr()->my_rpc_func1(stream, my_obj);
        rpc_sample_ptr()->my_rpc_func2(stream, my_obj_vec, 1);
        rpc_sample_ptr()->my_rpc_func3(stream, "hello world!", 16);
    }
}

/* 服务端 - 客户端回调 */
void client_cb(kchannel_ref_t* channel, knet_channel_cb_event_e e) {
    static int i = 0;
    kstream_t* stream = knet_channel_ref_get_stream(channel);
    if (e & channel_cb_event_recv) { /* 有数据可以读 */
        for (;;) {
            int error = rpc_sample_ptr()->rpc_proc(stream);
            if (error == error_ok) {
                if (++i >= 3) {
                    knet_loop_exit(knet_channel_ref_get_loop(channel));
                    break;
                }
            } else {
                break;
            }
        }
    }
}
    
/* 监听者回调 */
void acceptor_cb(kchannel_ref_t* channel, knet_channel_cb_event_e e) {
    if (e & channel_cb_event_accept) { /* 新连接 */
        /* 设置回调 */
        knet_channel_ref_set_cb(channel, client_cb);
    }
}

int main() {
    /* 创建循环 */
    kloop_t* loop = knet_loop_create();
    /* 创建客户端 */
    kchannel_ref_t* connector = knet_loop_create_channel(loop, 8, 1024);
    /* 创建监听者 */
    kchannel_ref_t* acceptor = knet_loop_create_channel(loop, 8, 1024);
    /* 设置回调 */
    knet_channel_ref_set_cb(connector, connector_cb);
    knet_channel_ref_set_cb(acceptor, acceptor_cb);
    /* 监听 */
    knet_channel_ref_accept(acceptor, 0, 80, 10);
    /* 连接 */
    knet_channel_ref_connect(connector, "127.0.0.1", 80, 5);
    /* 启动 */
    knet_loop_run(loop);
    /* 销毁, connector, acceptor不需要手动销毁 */
    knet_loop_destroy(loop);
    return 0;
}
