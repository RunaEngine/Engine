#include "io/fs.h"
#include <cstring>

namespace runa::runtime
{
    // ========== fs_request_c ==========
    fs_request_c::fs_request_c(uv_loop_t* loop) : loop_handler(loop), req(new uv_fs_t())
    {
        req->data = this;
    }

    fs_request_c::~fs_request_c()
    {
        cleanup_request();
    }

    void fs_request_c::cleanup_request()
    {
        if (req)
        {
            uv_fs_req_cleanup(req);
            delete req;
            req = nullptr;
        }
    }

    // ========== fs_stat_c ==========
    fs_stat_c::fs_stat_c(uv_loop_t* loop) : fs_request_c(loop)
    {
    }

    fs_stat_c::~fs_stat_c()
    {
    }

    int fs_stat_c::stat(const char* path, const std::function<void(ssize_t, const uv_stat_t*)>& cb)
    {
        stat_callback = cb;
        return uv_fs_stat(loop_handler, req, path, stat_cb);
    }

    int fs_stat_c::fstat(uv_file file, const std::function<void(ssize_t, const uv_stat_t*)>& cb)
    {
        stat_callback = cb;
        return uv_fs_fstat(loop_handler, req, file, stat_cb);
    }

    int fs_stat_c::lstat(const char* path, const std::function<void(ssize_t, const uv_stat_t*)>& cb)
    {
        stat_callback = cb;
        return uv_fs_lstat(loop_handler, req, path, stat_cb);
    }

    int fs_stat_c::statfs(const char* path, const std::function<void(ssize_t, uv_statfs_t*)>& cb)
    {
        statfs_callback = cb;
        return uv_fs_statfs(loop_handler, req, path, statfs_cb);
    }

    void fs_stat_c::stat_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_stat_c*>(req->data);
        if (self->stat_callback)
        {
            self->stat_callback(req->result, &req->statbuf);
        }
        self->cleanup_request();
    }

    void fs_stat_c::statfs_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_stat_c*>(req->data);
        if (self->statfs_callback)
        {
            self->statfs_callback(req->result, static_cast<uv_statfs_t*>(req->ptr));
        }
        self->cleanup_request();
    }

    // ========== fs_read_c ==========
    fs_read_c::fs_read_c(uv_loop_t* loop) : fs_request_c(loop), fd(-1)
    {
    }

    fs_read_c::~fs_read_c()
    {
        if (buffer) delete buffer;
    }

    int fs_read_c::read(const char* path, const std::function<void(ssize_t, const char*)>& cb)
    {
        callback = cb;
        return uv_fs_open(loop_handler, req, path, O_RDONLY, 0, open_cb);
    }

    int fs_read_c::read(uv_file file, size_t size, int64_t offset, const std::function<void(ssize_t, const char*)>& cb)
    {
        callback = cb;
        fd = file;
        buffer = new char[size];
        buf = uv_buf_init(buffer, size);
        return uv_fs_read(loop_handler, req, fd, &buf, 1, offset, read_cb);
    }

    void fs_read_c::open_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_read_c*>(req->data);
        
        if (req->result < 0)
        {
            if (self->callback)
            {
                self->callback(req->result, {});
            }
            close_cb(req);
            return;
        }

        self->fd = static_cast<uv_file>(req->result);
        uv_fs_req_cleanup(req);

        // Get file size
        uv_fs_fstat(self->loop_handler, req, self->fd, stat_cb);
    }

    void fs_read_c::stat_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_read_c*>(req->data);
        
        if (req->result < 0)
        {
            uv_fs_close(self->loop_handler, req, self->fd, close_cb);
            return;
        }

        size_t blksize = req->statbuf.st_blksize;
        uv_fs_req_cleanup(req);

        self->buffer = new char[blksize];
        self->buf = uv_buf_init(self->buffer, blksize);

        uv_fs_read(self->loop_handler, req, self->fd, &self->buf, 1, -1, read_cb);
    }

    void fs_read_c::read_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_read_c*>(req->data);
        ssize_t result = req->result;
        
        uv_fs_req_cleanup(req);

        if (result < 0)
        {
            if (self->callback)
            {
                self->callback(result, {});
            }
            uv_fs_close(self->loop_handler, req, self->fd, close_cb);
        }
        else if (result == 0)
        {
            if (self->callback)
            {
                self->callback(result, {});
            }
            uv_fs_close(self->loop_handler, req, self->fd, close_cb);
        }
        else
        {
            if (self->callback)
            {
                self->callback(result, self->buffer);
            }
            uv_fs_read(self->loop_handler, req, self->fd, &self->buf, 1, -1, read_cb);
        }
    }

    void fs_read_c::close_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_read_c*>(req->data);
        if (self->buffer) delete self->buffer;
        self->buffer = nullptr;
        self->fd = 0;
        uv_buf_init(nullptr, 0);
    }

    // ========== fs_write_c ==========
    fs_write_c::fs_write_c(uv_loop_t* loop) : fs_request_c(loop), fd(-1)
    {
    }

    fs_write_c::~fs_write_c()
    {
        if (buffer) free(buffer);
    }

    int fs_write_c::write(const char* path, const char* data, size_t size, int flags, const std::function<void(ssize_t)>& cb)
    {
        callback = cb;
        buf_size = size;
        buf_data = data;
        return uv_fs_open(loop_handler, req, path, flags, 0, open_cb);
    }

    int fs_write_c::write(uv_file file, const char* data, size_t size, int64_t offset, const std::function<void(ssize_t)>& cb)
    {
        callback = cb;
        if (buffer) free(buffer);
        buffer = (char*)malloc(size);
        memcpy(buffer, data, size);
        buf = uv_buf_init(buffer, size);
        return uv_fs_write(loop_handler, req, file, &buf, 1, offset, write_cb);
    }

    int fs_write_c::append(const char* path, const char* data, size_t size, const std::function<void(ssize_t)>& cb)
    {
        return write(path, data, size, O_WRONLY | O_CREAT | O_APPEND, cb);
    }

    void fs_write_c::open_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_write_c*>(req->data);

        self->fd = static_cast<uv_file>(req->result);
        uv_fs_req_cleanup(req);
        
        if (req->result < 0)
        {
            if (self->callback)
            {
                self->callback(req->result);
            }
            close_cb(req);
            return;
        }

        if (self->buffer) free(self->buffer);
        self->buffer = (char*)malloc(self->buf_size);
        memcpy(self->buffer, self->buf_data, self->buf_size);
        self->buf = uv_buf_init(self->buffer, self->buf_size);
        uv_fs_write(self->loop_handler, req, self->fd, &self->buf, 1, -1, write_cb);
    }

    void fs_write_c::write_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_write_c*>(req->data);
        ssize_t result = req->result;
        
        uv_fs_req_cleanup(req);

        if (self->callback)
        {
            self->callback(result);
        }

        uv_fs_close(self->loop_handler, req, self->fd, close_cb);
    }

    void fs_write_c::close_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_write_c*>(req->data);
        if (self->buffer)
        {
            free(self->buffer);
            self->buffer = nullptr;
        }
        if (self->buf_data) self->buf_data = nullptr;
        self->buf_size = 0;
        self->fd = 0;
        uv_buf_init(nullptr, 0);
    }

    // ========== fs_file_c ==========
    fs_file_c::fs_file_c(uv_loop_t* loop) : fs_request_c(loop)
    {
    }

    fs_file_c::~fs_file_c()
    {
    }

    int fs_file_c::open(const char* path, int flags, int mode, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_open(loop_handler, req, path, flags, mode, generic_cb);
    }

    int fs_file_c::close(uv_file file, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_close(loop_handler, req, file, generic_cb);
    }

    int fs_file_c::unlink(const char* path, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_unlink(loop_handler, req, path, generic_cb);
    }

    int fs_file_c::rename(const char* path, const char* new_path, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_rename(loop_handler, req, path, new_path, generic_cb);
    }

    int fs_file_c::chmod(const char* path, int mode, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_chmod(loop_handler, req, path, mode, generic_cb);
    }

    int fs_file_c::fchmod(uv_file file, int mode, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_fchmod(loop_handler, req, file, mode, generic_cb);
    }

    int fs_file_c::fsync(uv_file file, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_fsync(loop_handler, req, file, generic_cb);
    }

    int fs_file_c::fdatasync(uv_file file, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_fdatasync(loop_handler, req, file, generic_cb);
    }

    int fs_file_c::ftruncate(uv_file file, int64_t offset, const std::function<void(uv_fs_t*)>& cb)
    {
        callback = cb;
        return uv_fs_ftruncate(loop_handler, req, file, offset, generic_cb);
    }

    void fs_file_c::generic_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_file_c*>(req->data);
        if (self->callback)
        {
            self->callback(req);
        }
        delete self;
    }

    // ========== fs_dir_c ==========
    fs_dir_c::fs_dir_c(uv_loop_t* loop) : fs_request_c(loop)
    {
    }

    fs_dir_c::~fs_dir_c()
    {
    }

    int fs_dir_c::mkdir(const char* path, int mode, const std::function<void(ssize_t)>& cb)
    {
        simple_callback = cb;
        return uv_fs_mkdir(loop_handler, req, path, mode, simple_cb);
    }

    int fs_dir_c::rmdir(const char* path, const std::function<void(ssize_t)>& cb)
    {
        simple_callback = cb;
        return uv_fs_rmdir(loop_handler, req, path, simple_cb);
    }

    int fs_dir_c::scandir(const char* path, int flags, const std::function<void(ssize_t, const std::vector<uv_dirent_t>&)>& cb)
    {
        scandir_callback = cb;
        return uv_fs_scandir(loop_handler, req, path, flags, scandir_cb);
    }

    int fs_dir_c::opendir(const char* path, const std::function<void(ssize_t, uv_dir_t*)>& cb)
    {
        opendir_callback = cb;
        return uv_fs_opendir(loop_handler, req, path, opendir_cb);
    }

    int fs_dir_c::readdir(uv_dir_t* dir, const std::function<void(ssize_t, const std::vector<uv_dirent_t>&)>& cb)
    {
        scandir_callback = cb;
        return uv_fs_readdir(loop_handler, req, dir, readdir_cb);
    }

    int fs_dir_c::closedir(uv_dir_t* dir, const std::function<void(ssize_t)>& cb)
    {
        simple_callback = cb;
        return uv_fs_closedir(loop_handler, req, dir, simple_cb);
    }

    void fs_dir_c::simple_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_dir_c*>(req->data);
        ssize_t result = req->result;
        uv_fs_req_cleanup(req);

        if (self->simple_callback)
        {
            self->simple_callback(result);
        }
    }

    void fs_dir_c::scandir_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_dir_c*>(req->data);
        
        std::vector<uv_dirent_t> entries;
        ssize_t result = req->result;

        if (req->result >= 0)
        {
            uv_dirent_t entry;
            while (uv_fs_scandir_next(req, &entry) != UV_EOF)
            {
                entries.push_back(entry);
            }
        }

        uv_fs_req_cleanup(req);

        if (self->scandir_callback)
        {
            self->scandir_callback(result, entries);
        }
    }

    void fs_dir_c::opendir_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_dir_c*>(req->data);
        if (self->opendir_callback)
        {
            self->opendir_callback(req->result, static_cast<uv_dir_t*>(req->ptr));
        }
        uv_fs_req_cleanup(req);
    }

    void fs_dir_c::readdir_cb(uv_fs_t* req)
    {
        auto* self = static_cast<fs_dir_c*>(req->data);

        uv_dir_t* dir = static_cast<uv_dir_t*>(req->ptr);
        std::vector<uv_dirent_t> entries;

        if (req->result > 0)
        {

            for (size_t i = 0; i < dir->nentries; i++)
            {
                entries.push_back(dir->dirents[i]);
            }

            if (self->scandir_callback)
            {
                self->scandir_callback(req->result, entries);
            }
        }
        else if (req->result == 0)
        {
            if (self->scandir_callback)
            {
                self->scandir_callback(req->result, {});
            }

            self->closedir(static_cast<uv_dir_t*>(req->ptr), nullptr);
        }
        else
        {
            if (self->scandir_callback)
            {
                self->scandir_callback(req->result, {});
            }

            self->closedir(static_cast<uv_dir_t*>(req->ptr), nullptr);
        }

        uv_fs_req_cleanup(req);
    }
}