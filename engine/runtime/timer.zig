const runtime = @import("runtime");
const sdl = @import("sdl3");

pub const Time = struct {
    currentTimeNS: u64,
    deltaTimeNS: u64,

    pub fn init() Time {
        var self: Time = undefined;
        self.currentTimeNS = 0;
        self.deltaTimeNS = 0;

        return self;
    }
    pub fn updateCurrentTime(self: *Time) void {
        self.currentTimeNS = sdl.timer.getNanosecondsSinceInit();
    }
    pub fn updateEndTime(self: *Time) void {
        self.deltaTimeNS = sdl.timer.getNanosecondsSinceInit() - self.currentTimeNS;
    }
    pub fn elapsedNS(self: *Time) u64 {
        return sdl.timer.getNanosecondsSinceInit() - self.currentTimeNS;
    }
    pub fn deltaNS(self: *Time) u64 {
        return self.deltaTimeNS;
    }
    pub fn delta(self: *Time) f64 {
        return @as(f64, @floatFromInt(self.deltaTimeNS)) / 1_000_000_000.0;
    }
};
