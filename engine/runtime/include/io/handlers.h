#pragma once

#include <SDL3/SDL.h>
#include <uv.h>
#include <functional>
#include <memory>

namespace runa::runtime
{
    enum run_mode_t : uint8_t
    {
        POLL = 0,
        WAIT
    };

    class events_c
    {
    public:
        events_c() = default;

        void run(run_mode_t mode);

        bool push(SDL_Event &user_event);

        std::function<void(SDL_Event&)> callback;
    private:
        SDL_Event e;
    };

    class timer_c
    {
    public:
        timer_c() = default;    

        uint32_t add(uint32_t interval);

        uint32_t addNS(uint64_t interval);

        bool remove();

        uint32_t get_id();

        std::function<void()> callback;
    private:
        SDL_TimerID id = 0;

        static uint32_t timer_cb(void *userdata, SDL_TimerID timerID, Uint32 interval);
        static uint64_t ns_timer_cb(void *userdata, SDL_TimerID timerID, Uint64 interval);
    };

    class loop_c
    {
    public:
        loop_c();
        explicit loop_c(uv_loop_t* existing_loop);
        ~loop_c();

        int close();
        int run(uv_run_mode mode = UV_RUN_DEFAULT);
        void stop();

        uv_loop_t* get() const;
        int is_alive() const;

        loop_c(const loop_c&) = delete;
        loop_c& operator=(const loop_c&) = delete;
        loop_c(loop_c&&) noexcept;
        loop_c& operator=(loop_c&&) noexcept;

    private:
        uv_loop_t* loop = nullptr;
        bool is_owner;
    };

    template <typename T>
    class handler_c
    {
    public:
        handler_c();
        virtual ~handler_c();

        T* get();
        const T* get() const;

        bool is_active() const;
        bool is_closing() const;

        void close();
        void close(std::function<void()> cb);

        void ref();
        void unref();
        bool has_ref() const;

        handler_c(const handler_c&) = delete;
        handler_c& operator=(const handler_c&) = delete;

    protected:
        T handle;
        std::function<void()> callback;
        std::function<void()> close_callback;

        static void close_cb(uv_handle_t* h);
        
        void check_error(int result);
    };

    class async_c : public handler_c<uv_async_t>
    {
    public:
        async_c(loop_c& loop, std::function<void()> cb);
        async_c(uv_loop_t* loop, std::function<void()> cb);

        void send();

    private:
        static void async_cb(uv_async_t* handle);
    };

    class signal_c : public handler_c<uv_signal_t>
    {
    public:
        explicit signal_c(loop_c& loop);
        explicit signal_c(uv_loop_t* loop);

        void start(int signum, std::function<void(int)> cb);
        void start_oneshot(int signum, std::function<void(int)> cb);
        void stop();

        int get_signum() const;

    private:
        std::function<void(int)> signal_callback;
        static void signal_cb(uv_signal_t* handle, int signum);
    };

    class thread_c
    {
    public:
        thread_c();
        ~thread_c();

        int create(const std::function<void()>& cb);
        int join();
        int detach();

        thread_c(const thread_c&) = delete;
        thread_c& operator=(const thread_c&) = delete;
    private:
        uv_thread_t thread;

        std::function<void()> callback;

        static void thread_cb(void* arg);
    };

    class work_c
    {
    public:
        work_c(loop_c& loop);
        work_c(uv_loop_t* loop);
        ~work_c();

        int queue(
            std::function<void()> work_cb,
            std::function<void(int)> after_cb
        );

        work_c(const work_c&) = delete;
        work_c& operator=(const work_c&) = delete;

    private:
        uv_loop_t* loop_handler;
        uv_work_t* req;

        std::function<void()> work_callback;
        std::function<void(int)> after_callback;

        static void _work_cb(uv_work_t* req);
        static void after_work_cb(uv_work_t* req, int status);
    };
}