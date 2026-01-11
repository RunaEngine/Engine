const std = @import("std");
const sources = @import("stb.zon");

pub fn build(
    b: *std.Build,
    target: *std.Build.ResolvedTarget,
    optimize: *const std.builtin.OptimizeMode,
    module: *std.Build.Module,
    exe: *std.Build.Step.Compile,
) !*std.Build.Step.Compile {
    const upstream = b.dependency("stb", .{});
    const lib = b.addLibrary(.{
        .name = "stb",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target.*,
            .optimize = optimize.*,
            .link_libc = true,
        }),
    });

    lib.root_module.addIncludePath(upstream.path(""));
    lib.root_module.addCSourceFiles(.{ .root = upstream.path(""), .files = &sources.generic });
    lib.root_module.addCSourceFile(.{ .file = b.path("engine/thirdparty/stb/src/stb.c") });

    module.addIncludePath(upstream.path(""));
    module.linkLibrary(lib);
    exe.root_module.addIncludePath(upstream.path(""));
    exe.root_module.linkLibrary(lib);
    b.installArtifact(lib);

    return lib;
}
