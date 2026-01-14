const std = @import("std");
const zm = @import("zmath");

pub const math = struct {
    pub fn vecAngle(a: zm.Vec, b: zm.Vec) f32 {
        const dot = zm.dot3(a, b)[0];       // produto escalar como escalar
        const lenA = zm.length3(a)[0];      // magnitude de a
        const lenB = zm.length3(b)[0];      // magnitude de b

        if (lenA == 0 or lenB == 0) return 0.0;

        const cosTheta = dot / (lenA * lenB);
        const clamped = std.math.clamp(cosTheta, -1.0, 1.0);

        return std.math.acos(clamped);
    }
};