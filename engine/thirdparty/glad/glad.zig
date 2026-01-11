const std = @import("std");
const sources = @import("glad.zon");

pub fn build(
    b: *std.Build,
    target: *std.Build.ResolvedTarget,
    optimize: *const std.builtin.OptimizeMode,
    module: *std.Build.Module,
    exe: *std.Build.Step.Compile,
) *std.Build.Step.Compile {
    const vulkan_upstream = b.dependency("vulkan", .{});
    const lib = b.addLibrary(.{
        .name = "glad",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target.*,
            .optimize = optimize.*,
            .link_libc = true,
        }),
    });

    // ensure both the library and the consumer modules/exe see the headers
    lib.root_module.addIncludePath(vulkan_upstream.path("include"));
    lib.root_module.addIncludePath(b.path("engine/thirdparty/glad/include"));

    module.addIncludePath(vulkan_upstream.path("include"));
    exe.addIncludePath(vulkan_upstream.path("include"));
    module.addIncludePath(b.path("engine/thirdparty/glad/include"));
    exe.addIncludePath(b.path("engine/thirdparty/glad/include"));
    lib.root_module.addCSourceFiles(.{ .root = b.path("engine/thirdparty/glad/src"), .files = &sources.generic });

    module.linkLibrary(lib);
    exe.root_module.linkLibrary(lib);
    b.installArtifact(lib);

    return lib;
}
