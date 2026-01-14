const sdl = @import("sdl3");

pub fn log(comptime fmt: []const u8, args: anytype) void {
    sdl.log.log(fmt, args) catch {};
}

pub fn success(comptime fmt: []const u8, args: anytype) void {
    sdl.log.log("\x1b[32m", .{}) catch {};
    sdl.log.log(fmt, args) catch {};
    sdl.log.log("\x1b[0m\n", .{}) catch {};
}

pub fn warn(comptime fmt: []const u8, args: anytype) void {
    sdl.log.log("\x1b[33m", .{}) catch {};
    sdl.log.log(fmt, args) catch {};
    sdl.log.log("\x1b[0m\n", .{}) catch {};
}

pub fn err(comptime fmt: []const u8, args: anytype) void {
    sdl.log.log("\x1b[31m", .{}) catch {};
    sdl.log.log(fmt, args) catch {};
    sdl.log.log("\x1b[0m\n", .{}) catch {};
}

pub fn sdlErr() void {
    if (sdl.errors.get()) |errLog| {
        sdl.log.log("\x1b[31m{s}\x1b[0m\n", .{errLog}) catch {};
    }
}
