#pragma once

#include <uv.h>
#include <functional>
#include <memory>
#include <stdexcept>

namespace runa::runtime
{
    // Exceção customizada para erros da libuv
    class uv_error : public std::runtime_error
    {
    public:
        explicit uv_error(int error_code)
            : std::runtime_error(uv_strerror(error_code))
            , code(error_code)
        {
        }

        int error_code() const { return code; }

    private:
        int code;
    };

    // ========== Loop ==========
    class loop_c
    {
    public:
        loop_c();
        explicit loop_c(uv_loop_t* existing_loop); // Wrapper para loop existente
        ~loop_c();

        void close();
        int run(uv_run_mode mode = UV_RUN_DEFAULT);
        void stop();

        uv_loop_t* get() const;
        bool is_alive() const;
        uint64_t now() const;
        void update_time();

        // Backend fd (Unix) - para integração com poll
        int backend_fd() const;
        int backend_timeout() const;

        loop_c(const loop_c&) = delete;
        loop_c& operator=(const loop_c&) = delete;
        loop_c(loop_c&&) noexcept;
        loop_c& operator=(loop_c&&) noexcept;

    private:
        uv_loop_t* loop;
        bool is_owner;
    };

    // ========== Base Handler ==========
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

        // Referência para o handle (permite operações genéricas)
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

    // ========== Timer ==========
    class timer_c : public handler_c<uv_timer_t>
    {
    public:
        explicit timer_c(loop_c& loop);
        explicit timer_c(uv_loop_t* loop);

        void start(std::function<void()> cb, uint64_t timeout, uint64_t repeat = 0);
        void stop();
        void again(); // Restart com mesmo repeat

        void set_repeat(uint64_t repeat);
        uint64_t get_repeat() const;
        uint64_t get_due_in() const;

    private:
        static void timer_cb(uv_timer_t* handle);
    };

    // ========== Prepare ==========
    class prepare_c : public handler_c<uv_prepare_t>
    {
    public:
        explicit prepare_c(loop_c& loop);
        explicit prepare_c(uv_loop_t* loop);

        void start(std::function<void()> cb);
        void stop();

    private:
        static void prepare_cb(uv_prepare_t* handle);
    };

    // ========== Idle ==========
    class idle_c : public handler_c<uv_idle_t>
    {
    public:
        explicit idle_c(loop_c& loop);
        explicit idle_c(uv_loop_t* loop);

        void start(std::function<void()> cb);
        void stop();

    private:
        static void idle_cb(uv_idle_t* handle);
    };

    // ========== Check ==========
    class check_c : public handler_c<uv_check_t>
    {
    public:
        explicit check_c(loop_c& loop);
        explicit check_c(uv_loop_t* loop);

        void start(std::function<void()> cb);
        void stop();

    private:
        static void check_cb(uv_check_t* handle);
    };

    // ========== Async (Thread-safe) ==========
    class async_c : public handler_c<uv_async_t>
    {
    public:
        async_c(loop_c& loop, std::function<void()> cb);
        async_c(uv_loop_t* loop, std::function<void()> cb);

        void send();

    private:
        static void async_cb(uv_async_t* handle);
    };

    // ========== Signal ==========
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

    // ========== Thread ==========

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

    // ========== Work (Thread Pool) ==========
    class work_c
    {
    public:
        work_c(loop_c& loop);
        work_c(uv_loop_t* loop);
        ~work_c();

        void queue(
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

    // ========== Utilities ==========
    
    // Obter versão da libuv
    inline unsigned int version()
    {
        return uv_version();
    }

    inline const char* version_string()
    {
        return uv_version_string();
    }

    // Obter loop padrão (default)
    inline uv_loop_t* default_loop()
    {
        return uv_default_loop();
    }

    // Suspender thread
    inline void sleep(unsigned int msec)
    {
        uv_sleep(msec);
    }
}