const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn log(msg: []const u8) void {
    c.SDL_Log("%*s\n", msg.len, msg.ptr);
}

pub fn success(msg: []const u8) void {
    c.SDL_Log("\x1b[32m%*s\x1b[0m\n", msg.len, msg.ptr);
}

pub fn warn(msg: []const u8) void {
    c.SDL_Log("\x1b[33m%*s\x1b[0m\n", msg.len, msg.ptr);
}

pub fn err(msg: []const u8) void {
    c.SDL_Log("\x1b[31m%*s\x1b[0m\n", msg.len, msg.ptr);
}

pub fn sdlErr() void {
    c.SDL_Log("\x1b[31m%s\x1b[0m\n", c.SDL_GetError());
}
