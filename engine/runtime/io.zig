const std = @import("std");
const runtime = @import("runtime.zig");
const log = runtime.log;
const sdl = @import("sdl3");

pub const Mode = enum(u8) { pool = 0, wait = 1 };
pub const EventCallback = ?*const fn (event: sdl.events.Event, ctx: ?*anyopaque) void;

pub const Event = struct {
    callback: EventCallback,
    context: ?*anyopaque,

    pub fn init() Event {
        var self: Event = undefined;
        self.callback = null;
        self.context = null;
        return self;
    }

    pub fn setCallback(self: *Event, cb: EventCallback, ctx: ?*anyopaque) void {
        self.callback = cb;
        self.context = ctx;
    }

    pub fn run(self: *Event, wait: bool) void {
        const input = &runtime.input;

        if (wait) sdl.events.wait() catch {};
        while (sdl.events.poll()) |event| {
            input.updateEvent(event);
            if (self.callback) |callback| {
                callback(event, self.context);
            }
        }
    }
};

pub fn readFile(filepath: [:0]const u8, mode: sdl.io_stream.FileMode) ![]u8 {
    const allocator = runtime.defaultAllocator();

    if (filepath.len == 0) {
        log.err("Error: Path is empty", .{});
        return error.PathNull;
    }

    var file = sdl.io_stream.Stream.initFromFile(filepath, mode) catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };
    defer file.deinit() catch {
        log.sdlErr();
    };

    _ = file.seek(0, .end) catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    const filesize = file.tell() catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    _ = file.seek(0, .set) catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    const buffer = allocator.alloc(u8, @intCast(filesize)) catch |err| {
        log.err("{any}", .{@errorName(err)});
        return sdl.errors.Error.SdlError;
    };

    var eof = false;
    while (!eof) {
        const buf = file.read(buffer) catch {
            allocator.free(buffer);
            log.sdlErr();
            return sdl.errors.Error.SdlError;
        };

        if (buf == null) {
            eof = true;
        }
    }

    return buffer;
}

pub fn readTextFile(filepath: [:0]const u8) ![:0]u8 {
    const allocator = runtime.defaultAllocator();

    if (std.mem.len(filepath) == 0) {
        log.err("Error: Path is empty", .{});
        return error.PathNull;
    }

    var file = sdl.io_stream.Stream.initFromFile(filepath, .read_text) catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };
    defer file.deinit();

    const filesize = file.tell() catch {
        log.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    const buffer = allocator.alloc(u8, @intCast(filesize)) catch |err| {
        log.err("{any}", @errorName(err));
        return sdl.errors.Error.SdlError;
    };
    defer allocator.free(buffer);

    var eof = false;
    while (!eof) {
        const buf = file.read(buffer) catch {
            allocator.free(buffer);
            log.sdlErr();
            return sdl.errors.Error.SdlError;
        };

        if (buf == null) {
            eof = true;
        }
    }

    const dupezBuf = allocator.dupeZ(u8, buffer) catch |err| {
        log.err("{any}", .{@errorName(err)});
        return err;
    };

    return dupezBuf;
}
