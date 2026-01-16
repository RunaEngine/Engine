const std = @import("std");
const runtime = @import("runtime.zig");
const allocator = runtime.defaultAllocator();
const logs = runtime.utils.logs;
const inputSystem = &runtime.inputSystem;
const task = &runtime.task;
const c = @cImport({
    @cInclude("uv.h");
});

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
        if (wait) sdl.events.wait() catch {};
        while (sdl.events.poll()) |event| {
            inputSystem.updateEvent(event);
            if (self.callback) |callback| {
                callback(event, self.context);
            }
        }
    }
};

pub fn readFile(filepath: []const u8, mode: sdl.io_stream.FileMode) ![]u8 {
    _ = mode;
    if (filepath.len == 0) {
        logs.err("Error: Path is empty", .{});
        return error.PathNull;
    }

    var file = std.fs.cwd().openFile(filepath, .{}) catch |err| {
        logs.err("Failed to open file: {any}", .{@errorName(err)});
        return err;
    };
    defer file.close();

    const filesize = try file.getEndPos();

    const buffer = allocator.alloc(u8, filesize) catch |err| {
        logs.err("{any}", .{@errorName(err)});
        return err;
    };

    const bytes_read = try file.readAll(buffer);

    if (bytes_read != filesize) {
        allocator.free(buffer);
        logs.err("Failed to read entire file", .{});
        return error.IncompleteRead;
    }

    return buffer;
}
