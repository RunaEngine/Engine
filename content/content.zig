const std = @import("std");

pub fn afterBuild(b: *std.Build) !void {
    _ = b;

    const src_path = "content/resources";
    const dst_path = "zig-out/bin/resources";
    //const cwd = std.fs.cwd();
//
    //if (cwd.access(dst_path, .{})) |_| {
    //    return;
    //} else |err| {
    //    if (err != error.FileNotFound) return err;
    //}

    std.debug.print("Coping resources from '{s}' to '{s}'...\n", .{ src_path, dst_path });
    try copyDir(src_path, dst_path);
}

fn copyDir(src_path: []const u8, dst_path: []const u8) !void {
    const cwd = std.fs.cwd();

    try cwd.makePath(dst_path);

    var src_dir = try cwd.openDir(src_path, .{ .iterate = true });
    defer src_dir.close();

    var dst_dir = try cwd.openDir(dst_path, .{});
    defer dst_dir.close();

    var it = src_dir.iterate();
    while (try it.next()) |entry| {
        switch (entry.kind) {
            .file => {
                try src_dir.copyFile(entry.name, dst_dir, entry.name, .{});
            },
            .directory => {
                const allocator = std.heap.page_allocator;

                const sub_src = try std.fs.path.join(allocator, &[_][]const u8{ src_path, entry.name });
                defer allocator.free(sub_src);

                const sub_dst = try std.fs.path.join(allocator, &[_][]const u8{ dst_path, entry.name });
                defer allocator.free(sub_dst);

                try copyDir(sub_src, sub_dst);
            },
            else => {},
        }
    }
}
