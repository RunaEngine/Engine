#pragma once

#include <uv.h>
#include <SDL3/SDL.h>
#include <functional>

namespace runa::runtime
{
    class fs_request_c
    {
    public:
        fs_request_c(uv_loop_t* loop);
        virtual ~fs_request_c();

        uv_fs_t* get_request() { return req; }
        uv_loop_t* get_loop() { return loop_handler; }

    protected:
        uv_loop_t* loop_handler;
        uv_fs_t* req; 

        void cleanup_request();
    };

    class fs_stat_c : public fs_request_c
    {
    public:
        fs_stat_c(uv_loop_t* loop);
        ~fs_stat_c();

        int stat(const char* path, const std::function<void(ssize_t, const uv_stat_t*)>& cb);
        int fstat(uv_file file, const std::function<void(ssize_t, const uv_stat_t*)>& cb);
        int lstat(const char* path, const std::function<void(ssize_t, const uv_stat_t*)>& cb);
        int statfs(const char* path, const std::function<void(ssize_t, uv_statfs_t*)>& cb);

    private:
        std::function<void(ssize_t, const uv_stat_t*)> stat_callback;
        std::function<void(ssize_t, uv_statfs_t*)> statfs_callback;

        static void stat_cb(uv_fs_t* req);
        static void statfs_cb(uv_fs_t* req);
    };

    class fs_read_c : public fs_request_c
    {
    public:
        fs_read_c(uv_loop_t* loop);
        ~fs_read_c();

        int read(const char* path, const std::function<void(ssize_t, const char*)>& cb);
        int read(uv_file file, size_t size, int64_t offset, const std::function<void(ssize_t, const char*)>& cb);

    private:
        char* buffer = nullptr;
        uv_buf_t buf;
        uv_file fd;

        std::function<void(ssize_t, const char*)> callback;

        static void open_cb(uv_fs_t* req);
        static void stat_cb(uv_fs_t* req);
        static void read_cb(uv_fs_t* req);
        static void close_cb(uv_fs_t* req);
    };
}