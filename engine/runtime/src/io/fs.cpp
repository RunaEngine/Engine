#include "io/fs.h"

namespace runa::runtime
{
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
        buffer = (char*)malloc(size);
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
        delete self->buffer;
        self->fd = 0;
        uv_buf_init(nullptr, 0);
    }
}