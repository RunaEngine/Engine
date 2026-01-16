const std = @import("std");
const runtime = @import("runtime.zig");
const allocator = runtime.defaultAllocator();
const logs = runtime.utils.logs;
const sdl = @import("sdl3");
const za = @import("zalgebra");

pub const InputSystem = struct {
    keyboardevent: std.AutoHashMap(sdl.keycode.Keycode, sdl.events.Keyboard),
    mousevent: std.AutoHashMap(sdl.mouse.Button, sdl.events.MouseButton),

    pub fn init() InputSystem {
        var self: InputSystem = undefined;
        self.keyboardevent = .init(allocator);
        self.mousevent = .init(allocator);
        return self;
    }

    pub fn deinit(self: *InputSystem) void {
        self.keyboardevent.deinit();
        self.mousevent.deinit();
    }

    pub fn updateEvent(self: *InputSystem, event: sdl.events.Event) void {
        switch (event) {
            .key_up => |e| {
                if (e.key) |key| {
                    self.keyboardevent.put(key, e) catch |err| {
                        logs.err("{any}", .{@errorName(err)});
                    };
                }
            },
            .key_down => |e| {
                if (e.key) |key| {
                    self.keyboardevent.put(key, e) catch |err| {
                        logs.err("{any}", .{@errorName(err)});
                    };
                }
            },
            .mouse_button_up => |e| {
                self.mousevent.put(e.button, e) catch |err| {
                    logs.err("{any}", .{@errorName(err)});
                };
            },
            .mouse_button_down => |e| {
                self.mousevent.put(e.button, e) catch |err| {
                    logs.err("{any}", .{@errorName(err)});
                };
            },
            else => {}
        }
    }

    pub fn mouseButtonPressed(self: *InputSystem, button: sdl.mouse.Button) bool {
        if (self.mousevent.get(button)) |event| {
            return event.down;
        }
        return false;
    }

    pub fn keyPressed(self: *InputSystem, keyCode: sdl.keycode.Keycode) bool {
        if (self.keyboardevent.get(keyCode)) |event| {
            return event.down;
        }
        return false;
    }

    pub fn axis(self: *InputSystem, positive: sdl.keycode.Keycode, negative: sdl.keycode.Keycode) f32 {
        var val: f32 = 0.0;

        if (self.keyboardevent.get(positive)) |event| {
            if (event.down) val += 1.0;
        }
        if (self.keyboardevent.get(negative)) |event| {
            if (event.down) val -= 1.0;
        }

        val = std.math.clamp(val, -1.0, 1.0);

        return val;
    }

    pub fn vector(self: *InputSystem, positiveX: sdl.keycode.Keycode, negativeX: sdl.keycode.Keycode, positiveY: sdl.keycode.Keycode, negativeY: sdl.keycode.Keycode) za.Vec2 {
        var vec: za.Vec2 = za.Vec2.zero();

        if (self.keyboardevent.get(positiveX)) |event| {
            if (event.down) vec.xMut().* += 1.0;
        }
        if (self.keyboardevent.get(negativeX)) |event| {
            if (event.down) vec.xMut().* -= 1.0;
        }
        if (self.keyboardevent.get(positiveY)) |event| {
            if (event.down) vec.yMut().* += 1.0;
        }
        if (self.keyboardevent.get(negativeY)) |event| {
            if (event.down) vec.yMut().* -= 1.0;
        }
        
        vec.xMut().* = std.math.clamp(vec.x(), -1.0, 1.0);
        vec.yMut().* = std.math.clamp(vec.y(), -1.0, 1.0);

        return vec;
    }
};
