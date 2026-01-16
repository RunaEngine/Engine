const std = @import("std");
const runtime = @import("runtime.zig");
const allocator = runtime.defaultAllocator();
const sdl = @import("sdl3");

const PATH_SEPARATOR: u8 = if (std.Target.Os.Tag == .windows) '\\' else '/';
const PATH_SEPARATOR_OTHER: u8 =  if (std.Target.Os.Tag == .windows) '/' else '\\';

pub const env = struct {
    pub fn getVar(name: []const u8) ![]const u8 {
        const envi = try sdl.Environment.init(true);
        defer envi.deinit();
        
        const dupezName = try allocator.dupeZ(u8, name);
        defer allocator.free(dupezName);
        return std.mem.span(envi.getVariable(name));
    }

    pub fn getVars() !std.ArrayList([]const u8) {
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

    pub fn uvErr(code: i32) error{UvError}!void {
        if (code == 0) return;

        std.debug.print("\x1b[31mUv error code: {}\n{s} -> {s}\x1b[0m\n", .{code, c.uv_err_name(code), c.uv_strerror(code)});

        return error.UvError;
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

    pub fn join(paths: []const []const u8) ![]u8 {
        if (paths.len == 0) {
            logs.err("Error: array of path is empty", .{});
            return error{PathArrayIsNull};
        }

        var joinedPaths: u8 = undefined;
        for (paths) |p| {
            if (p.len == 0) continue;

            if (p[p.len] == PATH_SEPARATOR or p[p.len] == PATH_SEPARATOR_OTHER) {
                joinedPaths = std.mem.concat(allocator, u8, &.{p}) catch |err| {
                    logs.err("{any}", .{@errorName(err)});
                    return err;
                };
            } else joinedPaths = std.mem.concat(allocator, u8, &.{p, "/"}) catch |err| {
                logs.err("{any}", .{@errorName(err)});
                return err;
            };
        }

        return joinedPaths;
    }

    pub fn nativeSeparator(p: []const u8) []const u8 {
        const nativePath = p;
        std.mem.replaceScalar(u8, nativePath, PATH_SEPARATOR_OTHER, PATH_SEPARATOR);
        return nativePath;
    }
};