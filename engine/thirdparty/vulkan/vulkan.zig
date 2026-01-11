const std = @import("std");
const sources = @import("vulkan.zon");

pub fn build(
    b: *std.Build,
    module: *std.Build.Module,
    exe: *std.Build.Step.Compile,
) *std.Build.Step.Compile {
    const upstream = b.dependency("vulkan", .{});

    module.addIncludePath(upstream.path("include"));
    exe.addIncludePath(upstream.path("include"));

    return undefined;
}
