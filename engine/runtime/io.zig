const std = @import("std");
const runtime = @import("runtime.zig");
const log = runtime.log;
const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Mode = enum(u8) { pool = 0, wait = 1 };

pub const Event = struct {
    event: c.SDL_Event,
    callback: ?*const fn (e: c.SDL_Event, ctx: ?*anyopaque) void,
    context: ?*anyopaque,

    pub fn init() Event {
        var self: Event = undefined;
        self.event = undefined;
        self.callback = null;
        self.context = null;
        return self;
    }

    pub fn setCallback(self: *Event, cb: ?*const fn (e: c.SDL_Event, ctx: ?*anyopaque) void, ctx: ?*anyopaque) void {
        self.callback = cb;
        self.context = ctx;
    }

    pub fn run(self: *Event, mode: Mode) void {
        const input = &runtime.input;
        if (mode == .pool) {
            while (c.SDL_PollEvent(&self.event)) {
                input.updateEvent(@ptrCast(&self.event));
                if (self.callback) |callback| {
                    callback(self.event, self.context);
                }
            }
            return;
        }

        if (c.SDL_WaitEvent(&self.event)) {
            input.updateEvent(@ptrCast(&self.event));
            if (self.callback) |callback| {
                callback(self.event, self.context);
            }
        }
    }
};

pub fn readFile(filepath: []const u8) ?[]u8 {
    const allocator = runtime.defaultAllocator();

    if (filepath.len == 0) {
        log.err("Error: Path is empty");
        return null;
    }

    var file: *c.SDL_IOStream = undefined;
    if (c.SDL_IOFromFile(filepath.ptr, "rb")) |ioFile| {
        file = ioFile;
    } else {
        log.sdlErr();
        return null;
    }
    defer _ = c.SDL_CloseIO(file);

    if (c.SDL_SeekIO(file, 0, c.SDL_IO_SEEK_END) < 0) {
        log.sdlErr();
        return null;
    }

    const filesize = c.SDL_TellIO(file);
    if (filesize < 0) {
        log.sdlErr();
        return null;
    }

    if (c.SDL_SeekIO(file, 0, c.SDL_IO_SEEK_SET) < 0) {
        log.sdlErr();
        return null;
    }

    const buffer = allocator.alloc(u8, @intCast(filesize)) catch |err| {
        log.err(@errorName(err));
        return null;
    };

    const bytes_read = c.SDL_ReadIO(file, buffer.ptr, @intCast(filesize));
    if (bytes_read != filesize) {
        log.sdlErr();
        allocator.free(buffer);
        return null;
    }

    return buffer;
}
