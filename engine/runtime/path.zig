const std = @import("std");
const log = @import("log.zig");
const sdl = @import("sdl3");

pub fn basePath() [:0]const u8 {
    const path = sdl.filesystem.getBasePath() catch {
        log.sdlErr();
        return "";
    };
    return path;
}
