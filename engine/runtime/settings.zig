const runtime = @import("runtime.zig");
const render = &runtime.render;
const logs = runtime.utils.logs;
const sdl = @import("sdl3");

pub const WindowMode = enum(u8) { fullscreen = 0, windowed = 1 };

pub const VsyncMode = enum(i32) { adaptative = -1, disable = 0, enable = 1 };

pub const GameUserSettings = struct {
    framerateLimit: u16,

    pub fn init() GameUserSettings {
        const self: GameUserSettings = .{ .framerateLimit = 0 };

        return self;
    }

    pub fn setVsync(self: *GameUserSettings, mode: sdl.video.gl.SwapInterval) !void {
        _ = self;
        sdl.video.gl.setSwapInterval(mode) catch {
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        };
    }

    pub fn getVsync(self: *GameUserSettings) sdl.video.gl.SwapInterval {
        _ = self;
        const interval = sdl.video.gl.getSwapInterval() catch {
            logs.sdlErr();
            return .immediate;
        };

        return interval;
    }

    pub fn setWindowMode(self: *GameUserSettings, fullscreen: bool) void {
        _ = self;
        render.backend.window.setFullscreen(fullscreen) catch {
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        };
    }

    pub fn isFullScreen(self: *GameUserSettings) bool {
        _ = self;
        const mode = sdl.video.Window.getFullscreenMode(render.backend.window);

        if (mode) {
            return true;
        }

        return false;
    }

    pub fn setFramerateLimit(self: *GameUserSettings, limit: u16) void {
        self.framerateLimit = limit;
    }

    pub fn getFramerateLimit(self: *GameUserSettings) u16 {
        return self.framerateLimit;
    }
};
