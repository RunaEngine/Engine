#pragma once

#include <uv.h>
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

    class fs_write_c : public fs_request_c
    {
    public:
        fs_write_c(uv_loop_t* loop);
        ~fs_write_c();

        int write(const char* path, const char* data, size_t size, int flags, const std::function<void(ssize_t)>& cb);
        int write(uv_file file, const char* data, size_t size, int64_t offset, const std::function<void(ssize_t)>& cb);
        int append(const char* path, const char* data, size_t size, const std::function<void(ssize_t)>& cb);

    private:
        const char* buf_data = nullptr;
        char* buffer = nullptr;
        size_t buf_size;
        uv_buf_t buf;
        uv_file fd;

        std::function<void(ssize_t)> callback;

        static void open_cb(uv_fs_t* req);
        static void write_cb(uv_fs_t* req);
        static void close_cb(uv_fs_t* req);
    };

    class fs_file_c : public fs_request_c
    {
    public:
        fs_file_c(uv_loop_t* loop);
        ~fs_file_c();

        int open(const char* path, int flags, int mode, const std::function<void(uv_fs_t*)>& cb);
        int close(uv_file file, const std::function<void(uv_fs_t*)>& cb);
        int unlink(const char* path, const std::function<void(uv_fs_t*)>& cb);
        int rename(const char* path, const char* new_path, const std::function<void(uv_fs_t*)>& cb);
        int chmod(const char* path, int mode, const std::function<void(uv_fs_t*)>& cb);
        int fchmod(uv_file file, int mode, const std::function<void(uv_fs_t*)>& cb);
        int fsync(uv_file file, const std::function<void(uv_fs_t*)>& cb);
        int fdatasync(uv_file file, const std::function<void(uv_fs_t*)>& cb);
        int ftruncate(uv_file file, int64_t offset, const std::function<void(uv_fs_t*)>& cb);

    private:
        std::function<void(uv_fs_t* req)> callback;

        static void generic_cb(uv_fs_t*);
    };

    class fs_dir_c : public fs_request_c
    {
    public:
        fs_dir_c(uv_loop_t* loop);
        ~fs_dir_c();

        int mkdir(const char* path, int mode, const std::function<void(ssize_t)>& cb);
        int rmdir(const char* path, const std::function<void(ssize_t)>& cb);
        int scandir(const char* path, int flags, const std::function<void(ssize_t, const std::vector<uv_dirent_t>&)>& cb);
        int opendir(const char* path, const std::function<void(ssize_t, uv_dir_t*)>& cb);
        int readdir(uv_dir_t* dir, const std::function<void(ssize_t, const std::vector<uv_dirent_t>&)>& cb);
        int closedir(uv_dir_t* dir, const std::function<void(ssize_t)>& cb);

    private:
        std::function<void(ssize_t)> simple_callback;
        std::function<void(ssize_t, const std::vector<uv_dirent_t>&)> scandir_callback;
        std::function<void(ssize_t, uv_dir_t*)> opendir_callback;

        static void simple_cb(uv_fs_t* req);
        static void scandir_cb(uv_fs_t* req);
        static void opendir_cb(uv_fs_t* req);
        static void readdir_cb(uv_fs_t* req);
    };
}