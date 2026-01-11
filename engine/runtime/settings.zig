const runtime = @import("runtime.zig");
const render = runtime.render;
const log = runtime.log;
const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const WindowMode = enum(u8) { fullscreen = 0, windowed = 1 };

pub const VsyncMode = enum(i32) { adaptative = -1, disable = 0, enable = 1 };

pub const GameUserSettings = struct {
    framerateLimit: u16,

    pub fn init() GameUserSettings {
        const self: GameUserSettings = .{ .framerateLimit = 0 };

        return self;
    }

    pub fn setVsync(self: *GameUserSettings, mode: VsyncMode) void {
        _ = self;
        const status = c.SDL_GL_SetSwapInterval(@intFromEnum(mode));
        if (!status) log.sdlErr();
    }

    pub fn getVsync(self: *GameUserSettings) VsyncMode {
        _ = self;
        var swapInterval: i32 = 0;
        const status = c.SDL_GL_GetSwapInterval(&swapInterval);
        if (!status) log.sdlErr();
        return @enumFromInt(swapInterval);
    }

    pub fn setWindowMode(self: *GameUserSettings, mode: WindowMode) void {
        _ = self;
        const status = c.SDL_SetWindowFullscreen(render.backend.window, @intFromEnum(mode));
        if (!status) log.sdlErr();
    }

    pub fn getWindowMode(self: *GameUserSettings) WindowMode {
        _ = self;
        const mode = c.SDL_GetWindowFullscreenMode(render.backend.window);
        if (mode) |m| {
            if ((m.flags & c.SDL_WINDOW_FULLSCREEN) != 0) {
                return .fullscreen;
            }
        }
        return .windowed;
    }

    pub fn setFramerateLimit(self: *GameUserSettings, limit: u16) void {
        self.framerateLimit = limit;
    }

    pub fn getFramerateLimit(self: *GameUserSettings) u16 {
        return self.framerateLimit;
    }
};
