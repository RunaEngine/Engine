const std = @import("std");
const runtime = @import("runtime.zig");
const log = @import("log.zig");
const za = @import("zalgebra");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const InputSystem = struct {
    keyboardevent: std.AutoHashMap(u32, c.SDL_KeyboardEvent),
    mousevent: std.AutoHashMap(u8, c.SDL_MouseButtonEvent),

    pub fn init() InputSystem {
        const allocator = runtime.defaultAllocator();
        var self: InputSystem = undefined;
        self.keyboardevent = .init(allocator);
        self.mousevent = .init(allocator);

        return self;
    }

    pub fn updateEvent(self: *InputSystem, event: *const c.SDL_Event) void {
        if (event.type == c.SDL_EVENT_KEY_UP or event.type == c.SDL_EVENT_KEY_DOWN) {
            self.keyboardevent.put(event.key.scancode, event.key) catch |err| {
                log.err(@errorName(err));
            };
        } else if (event.type == c.SDL_EVENT_MOUSE_BUTTON_UP or event.type == c.SDL_EVENT_MOUSE_BUTTON_DOWN) {
            self.mousevent.put(event.button.button, event.button) catch |err| {
                log.err(@errorName(err));
            };
        }
    }

    pub fn mouseButtonPressed(self: *InputSystem, code: u8) bool {
        if (self.mousevent.get(code)) |event| {
            return event.down;
        }
        return false;
    }

    pub fn keyPressed(self: *InputSystem, scancode: u32) bool {
        if (self.keyboardevent.get(scancode)) |event| {
            return event.down;
        }
        return false;
    }

    pub fn axis(self: *InputSystem, positive: u32, negative: u32) f32 {
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

    pub fn vector(self: *InputSystem, positiveX: u32, negativeX: u32, positiveY: u32, negativeY: u32) za.Vec2 {
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
