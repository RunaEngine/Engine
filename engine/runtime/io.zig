const std = @import("std");
const runtime = @import("runtime.zig");
const logs = runtime.utils.logs;
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
        const input = &runtime.inputSystem;

        if (wait) sdl.events.wait() catch {};
        while (sdl.events.poll()) |event| {
            input.updateEvent(event);
            if (self.callback) |callback| {
                callback(event, self.context);
            }
        }
    }
};

pub fn readFile(filepath: []const u8, mode: sdl.io_stream.FileMode) ![]u8 {
    const allocator = runtime.defaultAllocator();

    if (filepath.len == 0) {
        logs.err("Error: Path is empty", .{});
        return error.PathNull;
    }

    const dupezPath = allocator.dupeZ(u8, filepath) catch |err| {
        logs.err("{any}", .{@errorName(err)});
        return err;
    };

    var file = sdl.io_stream.Stream.initFromFile(dupezPath, mode) catch {
        logs.sdlErr();
        return sdl.errors.Error.SdlError;
    };
    defer file.deinit() catch {
        logs.sdlErr();
    };

    allocator.free(dupezPath);

    _ = file.seek(0, .end) catch {
        logs.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    const filesize = file.tell() catch {
        logs.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    _ = file.seek(0, .set) catch {
        logs.sdlErr();
        return sdl.errors.Error.SdlError;
    };

    const buffer = allocator.alloc(u8, @intCast(filesize)) catch |err| {
        logs.err("{any}", .{@errorName(err)});
        return sdl.errors.Error.SdlError;
    };

    var eof = false;
    while (!eof) {
        const buf = file.read(buffer) catch {
            allocator.free(buffer);
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        };

        if (buf == null) {
            eof = true;
        }
    }

    return buffer;
}
