#include "io/handlers.h"
#include <utility>
#include <cstring>

namespace runa::runtime
{
   // ========== loop_c ==========
    loop_c::loop_c() : is_owner(true)
    {
        loop = new uv_loop_t;
        int status = uv_loop_init(loop);
        if (status < 0)
        {
            delete loop;
            throw uv_error(status);
        }
    }

    loop_c::loop_c(uv_loop_t* existing_loop) : loop(existing_loop), is_owner(false)
    {
        if (!loop)
        {
            throw std::invalid_argument("Loop cannot be null");
        }
    }

    loop_c::~loop_c()
    {
        if (is_owner && loop)
        {
            close();
            delete loop;
        }
    }

    loop_c::loop_c(loop_c&& other) noexcept
        : loop(other.loop), is_owner(other.is_owner)
    {
        other.loop = nullptr;
        other.is_owner = false;
    }

    loop_c& loop_c::operator=(loop_c&& other) noexcept
    {
        if (this != &other)
        {
            if (is_owner && loop)
            {
                close();
                delete loop;
            }
            
            loop = other.loop;
            is_owner = other.is_owner;
            
            other.loop = nullptr;
            other.is_owner = false;
        }
        return *this;
    }

    void loop_c::close()
    {
        if (loop)
        {
            int status = uv_loop_close(loop);
            if (status == UV_EBUSY)
            {
                // Walk through handles and close them
                uv_walk(loop, [](uv_handle_t* handle, void*)
                {
                    if (!uv_is_closing(handle))
                    {
                        uv_close(handle, nullptr);
                    }
                }, nullptr);
                
                // Try again after running the loop
                uv_run(loop, UV_RUN_DEFAULT);
                status = uv_loop_close(loop);
            }
            
            if (status < 0 && status != UV_EBUSY)
            {
                throw uv_error(status);
            }
        }
    }

    int loop_c::run(uv_run_mode mode)
    {
        return uv_run(loop, mode);
    }

    void loop_c::stop()
    {
        uv_stop(loop);
    }

    uv_loop_t* loop_c::get() const
    {
        return loop;
    }

    bool loop_c::is_alive() const
    {
        return uv_loop_alive(loop) != 0;
    }

    uint64_t loop_c::now() const
    {
        return uv_now(loop);
    }

    void loop_c::update_time()
    {
        uv_update_time(loop);
    }

    int loop_c::backend_fd() const
    {
        return uv_backend_fd(loop);
    }

    int loop_c::backend_timeout() const
    {
        return uv_backend_timeout(loop);
    }

    // ========== handler_c ==========
    template <typename T>
    handler_c<T>::handler_c()
    {
        memset(&handle, 0, sizeof(T));
        handle.data = this;
    }

    template <typename T>
    handler_c<T>::~handler_c()
    {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&handle)))
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&handle), close_cb);
        }
    }

    template <typename T>
    T* handler_c<T>::get()
    {
        return &handle;
    }

    template <typename T>
    const T* handler_c<T>::get() const
    {
        return &handle;
    }

    template <typename T>
    bool handler_c<T>::is_active() const
    {
        return uv_is_active(reinterpret_cast<const uv_handle_t*>(&handle)) != 0;
    }

    template <typename T>
    bool handler_c<T>::is_closing() const
    {
        return uv_is_closing(reinterpret_cast<const uv_handle_t*>(&handle)) != 0;
    }

    template <typename T>
    void handler_c<T>::close()
    {
        if (!is_closing())
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&handle), close_cb);
        }
    }

    template <typename T>
    void handler_c<T>::close(std::function<void()> cb)
    {
        close_callback = std::move(cb);
        close();
    }

    template <typename T>
    void handler_c<T>::ref()
    {
        uv_ref(reinterpret_cast<uv_handle_t*>(&handle));
    }

    template <typename T>
    void handler_c<T>::unref()
    {
        uv_unref(reinterpret_cast<uv_handle_t*>(&handle));
    }

    template <typename T>
    bool handler_c<T>::has_ref() const
    {
        return uv_has_ref(reinterpret_cast<const uv_handle_t*>(&handle)) != 0;
    }

    template <typename T>
    void handler_c<T>::close_cb(uv_handle_t* h)
    {
        auto* self = static_cast<handler_c<T>*>(h->data);
        if (self && self->close_callback)
        {
            self->close_callback();
        }
    }

    template <typename T>
    void handler_c<T>::check_error(int result)
    {
        if (result < 0)
        {
            throw uv_error(result);
        }
    }

    // Instantiate templates
    template class handler_c<uv_timer_t>;
    template class handler_c<uv_prepare_t>;
    template class handler_c<uv_idle_t>;
    template class handler_c<uv_check_t>;
    template class handler_c<uv_async_t>;
    template class handler_c<uv_signal_t>;

    // ========== timer_c ==========
    timer_c::timer_c(loop_c& loop)
    {
        check_error(uv_timer_init(loop.get(), &handle));
    }

    timer_c::timer_c(uv_loop_t* loop)
    {
        check_error(uv_timer_init(loop, &handle));
    }

    void timer_c::start(std::function<void()> cb, uint64_t timeout, uint64_t repeat)
    {
        callback = std::move(cb);
        check_error(uv_timer_start(&handle, timer_cb, timeout, repeat));
    }

    void timer_c::stop()
    {
        check_error(uv_timer_stop(&handle));
    }

    void timer_c::again()
    {
        check_error(uv_timer_again(&handle));
    }

    void timer_c::set_repeat(uint64_t repeat)
    {
        uv_timer_set_repeat(&handle, repeat);
    }

    uint64_t timer_c::get_repeat() const
    {
        return uv_timer_get_repeat(&handle);
    }

    uint64_t timer_c::get_due_in() const
    {
        return uv_timer_get_due_in(&handle);
    }

    void timer_c::timer_cb(uv_timer_t* h)
    {
        auto* self = static_cast<timer_c*>(h->data);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== prepare_c ==========
    prepare_c::prepare_c(loop_c& loop)
    {
        check_error(uv_prepare_init(loop.get(), &handle));
    }

    prepare_c::prepare_c(uv_loop_t* loop)
    {
        check_error(uv_prepare_init(loop, &handle));
    }

    void prepare_c::start(std::function<void()> cb)
    {
        callback = std::move(cb);
        check_error(uv_prepare_start(&handle, prepare_cb));
    }

    void prepare_c::stop()
    {
        check_error(uv_prepare_stop(&handle));
    }

    void prepare_c::prepare_cb(uv_prepare_t* h)
    {
        auto* self = static_cast<prepare_c*>(h->data);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== idle_c ==========
    idle_c::idle_c(loop_c& loop)
    {
        check_error(uv_idle_init(loop.get(), &handle));
    }

    idle_c::idle_c(uv_loop_t* loop)
    {
        check_error(uv_idle_init(loop, &handle));
    }

    void idle_c::start(std::function<void()> cb)
    {
        callback = std::move(cb);
        check_error(uv_idle_start(&handle, idle_cb));
    }

    void idle_c::stop()
    {
        check_error(uv_idle_stop(&handle));
    }

    void idle_c::idle_cb(uv_idle_t* h)
    {
        auto* self = static_cast<idle_c*>(h->data);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== check_c ==========
    check_c::check_c(loop_c& loop)
    {
        check_error(uv_check_init(loop.get(), &handle));
    }

    check_c::check_c(uv_loop_t* loop)
    {
        check_error(uv_check_init(loop, &handle));
    }

    void check_c::start(std::function<void()> cb)
    {
        callback = std::move(cb);
        check_error(uv_check_start(&handle, check_cb));
    }

    void check_c::stop()
    {
        check_error(uv_check_stop(&handle));
    }

    void check_c::check_cb(uv_check_t* h)
    {
        auto* self = static_cast<check_c*>(h->data);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== async_c ==========
    async_c::async_c(loop_c& loop, std::function<void()> cb)
    {
        callback = std::move(cb);
        check_error(uv_async_init(loop.get(), &handle, async_cb));
    }

    async_c::async_c(uv_loop_t* loop, std::function<void()> cb)
    {
        callback = std::move(cb);
        check_error(uv_async_init(loop, &handle, async_cb));
    }

    void async_c::send()
    {
        check_error(uv_async_send(&handle));
    }

    void async_c::async_cb(uv_async_t* h)
    {
        auto* self = static_cast<async_c*>(h->data);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== signal_c ==========
    signal_c::signal_c(loop_c& loop)
    {
        check_error(uv_signal_init(loop.get(), &handle));
    }

    signal_c::signal_c(uv_loop_t* loop)
    {
        check_error(uv_signal_init(loop, &handle));
    }

    void signal_c::start(int signum, std::function<void(int)> cb)
    {
        signal_callback = std::move(cb);
        check_error(uv_signal_start(&handle, signal_cb, signum));
    }

    void signal_c::start_oneshot(int signum, std::function<void(int)> cb)
    {
        signal_callback = std::move(cb);
        check_error(uv_signal_start_oneshot(&handle, signal_cb, signum));
    }

    void signal_c::stop()
    {
        check_error(uv_signal_stop(&handle));
    }

    int signal_c::get_signum() const
    {
        return handle.signum;
    }

    void signal_c::signal_cb(uv_signal_t* h, int signum)
    {
        auto* self = static_cast<signal_c*>(h->data);
        if (self && self->signal_callback)
        {
            self->signal_callback(signum);
        }
    }

    thread_c::thread_c()
    {
    }

    thread_c::~thread_c()
    {
    }

    // ========== Thread ==========

    int thread_c::create(const std::function<void()>& cb)
    {
        callback = cb;
        return uv_thread_create(&thread, thread_cb, this);
    }

    int thread_c::join()
    {
        return uv_thread_join(&thread);
    }

    int thread_c::detach()
    {
        return uv_thread_detach(&thread);
    }

    void thread_c::thread_cb(void* arg)
    {
        auto* self = static_cast<thread_c*>(arg);
        if (self && self->callback)
        {
            self->callback();
        }
    }

    // ========== work_c ==========
    work_c::work_c(loop_c& loop) : loop_handler(loop.get()), req(new uv_work_t())
    {
        req->data = this;
    }

    work_c::work_c(uv_loop_t* loop) : loop_handler(loop), req(new uv_work_t())
    {
        req->data = this;
    }

    work_c::~work_c()
    {
        delete req;
    }

    void work_c::queue(std::function<void()> work_cb, std::function<void(int)> after_cb)
    {
        work_callback = std::move(work_cb);
        after_callback = std::move(after_cb);
        
        int result = uv_queue_work(loop_handler, req, _work_cb, after_work_cb);
        if (result < 0)
        {
            throw uv_error(result);
        }
    }

    void work_c::_work_cb(uv_work_t* req)
    {
        auto* self = static_cast<work_c*>(req->data);
        if (self && self->work_callback)
        {
            self->work_callback();
        }
    }

    void work_c::after_work_cb(uv_work_t* req, int status)
    {
        auto* self = static_cast<work_c*>(req->data);
        if (self && self->after_callback)
        {
            self->after_callback(status);
        }
    }
}