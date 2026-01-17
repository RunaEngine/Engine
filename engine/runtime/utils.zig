const std = @import("std");
const runtime = @import("runtime.zig");
const allocator = runtime.defaultAllocator();
const sdl = @import("sdl3");
const c = @cImport({
    @cInclude("uv.h");
});

const PATH_SEPARATOR: u8 = if (std.Target.Os.Tag == .windows) '\\' else '/';
const PATH_SEPARATOR_OTHER: u8 =  if (std.Target.Os.Tag == .windows) '/' else '\\';

pub const env = struct {
    pub fn getVar(name: []const u8) ![]u8 {
        return std.process.getEnvVarOwned(allocator, name);
    }

    pub fn getVars() !std.ArrayList([]const u8) {
        var env_map = try std.process.getEnvMap(allocator);
        defer env_map.deinit();

        var vars = std.ArrayList([]const u8).init(allocator);
        var it = env_map.iterator();
        while (it.next()) |entry| {
            const pair = try std.fmt.allocPrint(allocator, "{s}={s}", .{ entry.key_ptr.*, entry.value_ptr.* });
            try vars.append(pair);
        }
        
        return vars;
    }

    pub fn getUserName() ![]u8 {
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
    pub fn basePath() ![]u8 {
        const exe_path = try std.fs.selfExePathAlloc(allocator);
        defer allocator.free(exe_path);
        const dir = std.fs.path.dirname(exe_path) orelse ".";
        return try allocator.dupe(u8, dir);
    }

    pub fn currentDir() ![]u8 {
        return try std.process.getCwdAlloc(allocator);
    }

    pub fn homeDir() ![]u8 {
        if (comptime std.Target.Os.Tag == .windows) {
            return try env.getVar("USERPROFILE");
        } else {
            return try env.getVar("HOME");
        }
    }

    pub fn prefPath(app: []const u8) ![]u8 {
        return (try std.fs.getAppDataDir(allocator, app)) orelse error.AppDataDirNotFound;
    }

    pub fn join(paths: []const []const u8) ![]u8 {
        if (paths.len == 0) {
            logs.err("Error: array of path is empty", .{});
            return error.PathArrayIsEmpty;
        }

        return std.fs.path.join(allocator, paths);
    }

    pub fn nativeSeparator(p: []const u8) []const u8 {
        const nativePath = p;
        std.mem.replaceScalar(u8, nativePath, PATH_SEPARATOR_OTHER, PATH_SEPARATOR);
        return nativePath;
    }
};