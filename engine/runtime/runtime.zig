const std = @import("std");
pub const gl = @import("opengl.zig");
pub const timer = @import("timer.zig");
pub const io = @import("io.zig");
pub const inputSystem = @import("input.zig");
pub const settings = @import("settings.zig");
pub const log = @import("log.zig");
pub const path = @import("path.zig");

pub fn defaultAllocator() std.mem.Allocator {
    return std.heap.c_allocator;
}

pub var gameUserSettings: settings.GameUserSettings = settings.GameUserSettings.init();

pub var time: timer.Time = timer.Time.init();

pub var input: inputSystem.InputSystem = inputSystem.InputSystem.init();

pub var event: io.Event = io.Event.init();

pub var render: gl.Render = gl.Render.init();

pub fn add(a: i32, b: i32) i32 {
    return a + b;
}

test "runtime test" {
    try std.testing.expect(add(3, 7) == 10);
}
