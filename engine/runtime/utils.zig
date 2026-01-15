const std = @import("std");
const runtime = @import("runtime.zig");
const sdl = @import("sdl3");

pub const env = struct {
    pub fn getVar(name: []const u8) ![:0]const u8 {
        const envi = try sdl.Environment.init(true);
        defer envi.deinit();

        return envi.getVariable(name);
    }

    pub fn getVars() !std.ArrayList([]const u8) {
        const allocator = runtime.defaultAllocator();
        const envi = try sdl.Environment.init(true);
        defer envi.deinit();

        const envVars = try envi.getVariables();
        var vars = std.ArrayList([]const u8).empty;
        
        for (envVars) |var_str| {
            try vars.append(allocator, var_str);
        }
        
        return vars;
    }

    pub fn getUserName() ![:0]const u8 {
        if (comptime std.Target.Os.Tag == .windows) {
            return try getVar("USERNAME");
        }

        return try getVar("HOSTNAME");
    }
};

pub const logs = struct {
    pub fn log(comptime fmt: []const u8, args: anytype) void {
        std.debug.print(fmt, args);
    }

    pub fn success(comptime fmt: []const u8, args: anytype) void {
        std.debug.print("\x1b[32m", .{});
        std.debug.print(fmt, args);
        std.debug.print("\x1b[0m\n", .{});
    }

    pub fn warn(comptime fmt: []const u8, args: anytype) void {
        std.debug.print("\x1b[33m", .{});
        std.debug.print(fmt, args);
        std.debug.print("\x1b[0m\n", .{});
    }

    pub fn err(comptime fmt: []const u8, args: anytype) void {
        std.debug.print("\x1b[31m", .{});
        std.debug.print(fmt, args);
        std.debug.print("\x1b[0m\n", .{});
    }

    pub fn sdlErr() void {
        if (sdl.errors.get()) |errLog| {
            std.debug.print("\x1b[31m{s}\x1b[0m\n", .{errLog});
        }
    }
};

pub const path = struct {
    pub fn basePath() ![:0]const u8 {
        return try sdl.filesystem.getBasePath();
    }

    pub fn currentDir() ![:0]u8 {
        return try sdl.filesystem.getCurrentDirectory();
    }

    pub fn homeDir() ![:0]u8 {
        if (comptime std.Target.Os.Tag == .windows) {
            return try env.getVar("USERPROFILE");
        }
        return try env.getVar("HOME");
    }

    pub fn prefPath(org: [:0]const u8, app: [:0]const u8) ![:0]u8 {
        return try sdl.filesystem.getPrefPath(org, app);
    }
};