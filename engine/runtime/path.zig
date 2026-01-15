const std = @import("std");
const runtime = @import("runtime.zig");
const logs = runtime.utils.logs;
const sdl = @import("sdl3");

pub fn basePath() [:0]const u8 {
    const path = sdl.filesystem.getBasePath() catch {
        logs.sdlErr();
        return "";
    };
    return path;
}
