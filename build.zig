const std = @import("std");
const thirdparty = @import("engine/thirdparty/thirdparty.zig");
const content = @import("content/content.zig");

pub fn build(b: *std.Build) !void {
    var target = b.standardTargetOptions(.{});
    if (target.result.os.tag == .windows) {
        target.result.abi = .msvc;
    }
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSafe });

    const mod = b.addModule("runtime", .{
        .root_source_file = b.path("engine/runtime/runtime.zig"),
        .target = target,
        .link_libc = true,
    });

    const exe = b.addExecutable(.{
        .name = "Runa",
        .root_module = b.createModule(.{
            .root_source_file = b.path("engine/runa/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "runtime", .module = mod },
            },
            .link_libc = true,
        }),
        .use_llvm = true
    });

    //const sdl = b.dependency("sdl", .{
    //    .optimize = optimize,
    //    .target = target,
    //});
    //
    //mod.linkLibrary(sdl.artifact("SDL3"));
    //exe.root_module.linkLibrary(sdl.artifact("SDL3"));

    const sdl3 = b.dependency("sdl3", .{
        .target = target,
        .optimize = optimize,
        .ext_image = true,
    });

    mod.addImport("sdl3", sdl3.module("sdl3"));
    exe.root_module.addImport("sdl3", sdl3.module("sdl3"));

    const zgl = b.dependency("zgl", .{
        .target = target,
        .optimize = optimize,
    });
    mod.addImport("zgl", zgl.module("zgl"));
    exe.root_module.addImport("zgl", zgl.module("zgl"));

    const zalgebra = b.dependency("zalgebra", .{
        .target = target,
        .optimize = optimize,
    });
    mod.addImport("zalgebra", zalgebra.module("zalgebra"));
    exe.root_module.addImport("zalgebra", zalgebra.module("zalgebra"));

    const libuv = b.dependency("libuv", .{
        .target = target,
        .optimize = optimize,
    });
    mod.linkLibrary(libuv.artifact("uv"));
    exe.root_module.linkLibrary(libuv.artifact("uv"));

    try thirdparty.build(b, &target, &optimize, mod, exe);

    // This declares intent for the executable to be installed into the
    // install prefix when running `zig build` (i.e. when executing the default
    // step). By default the install prefix is `zig-out/` but can be overridden
    // by passing `--prefix` or `-p`.
    b.installArtifact(exe);

    // This creates a top level step.
    const run_step = b.step("run", "Run the app");

    // This creates a RunArtifact step in the build graph.
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    // By making the run step depend on the default step, it will be run from the
    run_cmd.step.dependOn(b.getInstallStep());

    // This allows the user to pass arguments to the application in the build
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // Creates an executable that will run `test` blocks from the provided module.
    const mod_tests = b.addTest(.{
        .root_module = mod,
    });

    // A run step that will run the test executable.
    const run_mod_tests = b.addRunArtifact(mod_tests);

    // Creates an executable that will run `test` blocks from the executable's
    const exe_tests = b.addTest(.{
        .root_module = exe.root_module,
    });

    // A run step that will run the second test executable.
    const run_exe_tests = b.addRunArtifact(exe_tests);

    // A top level step for running all tests. dependOn can be called multiple
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_mod_tests.step);
    test_step.dependOn(&run_exe_tests.step);

    try content.afterBuild(b);
}
