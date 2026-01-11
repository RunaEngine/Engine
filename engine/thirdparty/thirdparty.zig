const std = @import("std");
const vulkan = @import("vulkan/vulkan.zig");
const glad = @import("glad/glad.zig");
const stb = @import("stb/stb.zig");

pub fn build(
    b: *std.Build,
    target: *std.Build.ResolvedTarget,
    optimize: *const std.builtin.OptimizeMode,
    module: *std.Build.Module,
    exe: *std.Build.Step.Compile,
) !void {
    _ = vulkan.build(b, module, exe);
    _ = glad.build(b, target, optimize, module, exe);
    _ = try stb.build(b, target, optimize, module, exe);
}
