const std = @import("std");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn currentPath() []const u8 {
    return std.mem.span(c.SDL_GetBasePath());
}
