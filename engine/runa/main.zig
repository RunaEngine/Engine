const std = @import("std");
const runtime = @import("runtime");
const String = @import("string").String;
const gl = runtime.gl;
const sdl = @import("sdl3");
const zgl = @import("zgl");
const za = @import("zalgebra");

var shouldClose = false;

const windowsSize = struct {
    w: usize,
    h: usize
};

fn onEvent(event: sdl.events.Event, ctx: ?*anyopaque) void {
    //_ = ctx;
    var context: *RenderContext = undefined;
    if (@as(?*RenderContext, @ptrCast(@alignCast(ctx)))) |c| {
        context = c;
    } else {
        return;
    }

    var w: i32 = 0;
    var h: i32 = 0;

    switch (event) {
        .quit => {
            shouldClose = true;
        },
        .window_resized => |e| {
            w = @intCast(e.width); 
            h = @intCast(e.height); 
            zgl.viewport(0, 0, @intCast(e.width), @intCast(e.height));
        },
        else => {}
    }

    context.cam.editorInput(event);
}

const RenderContext = struct { 
    shader: *gl.Shader, 
    vao: *gl.VertexArray, 
    ebo: *gl.ElementBuffer, 
    tex: *gl.Texture, 
    lightShader: *gl.Shader, 
    lightVao: *gl.VertexArray, 
    lightEbo: *gl.ElementBuffer, 
    cam: *gl.Camera 
};

fn onRender(ctx: ?*anyopaque, delta: f64) void {
    _ = delta;

    var context: *RenderContext = undefined;
    if (@as(?*RenderContext, @ptrCast(@alignCast(ctx)))) |c| {
        context = c;
    } else {
        return;
    }

    context.cam.editorTick();
    context.cam.updateMatrix(80.0, 0.1, 100);

    context.shader.use();
    zgl.uniform3f(zgl.getUniformLocation(context.shader.id, "camPos"), context.cam.position.x(), context.cam.position.y(), context.cam.position.z());
    context.cam.matrix(context.shader, "camMatrix");
    context.tex.bind();
    context.vao.bind();
    zgl.drawElements(.triangles, @as(usize, context.ebo.size / @sizeOf(u32)), .unsigned_int, 0);

    context.lightShader.use();
    context.cam.matrix(context.lightShader, "camMatrix");
    context.lightVao.bind();
    zgl.drawElements(.triangles, @as(usize, context.lightEbo.size / @sizeOf(u32)), .unsigned_int, 0);
}

pub fn main() !void {
    const allocator = runtime.defaultAllocator();
    try runtime.render.initBackend(.core);
    defer runtime.render.deinitBackend();

    //runtime.gameUserSettings.setVsync(.disable);
    //runtime.gameUserSettings.setFramerateLimit(300);

    const vertices: [176]f32 =
        .{
            -0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 0.0, 0.0,  0.0, -1.0,  0.0, // Bottom side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 5.0,  0.0, -1.0,  0.0, // Bottom side
             0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 5.0,  0.0, -1.0,  0.0, // Bottom side
             0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 5.0, 0.0,  0.0, -1.0,  0.0, // Bottom side
            -0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 0.0, 0.0, -0.8,  0.5,  0.0, // Left Side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 0.0, -0.8,  0.5,  0.0, // Left Side
             0.0, 0.8,  0.0, 0.92, 0.86, 0.76, 2.5, 5.0, -0.8,  0.5,  0.0, // Left Side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 0.0,  0.0,  0.5, -0.8, // Non-facing side
             0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 0.0,  0.0,  0.5, -0.8, // Non-facing side
             0.0, 0.8,  0.0, 0.92, 0.86, 0.76, 2.5, 5.0,  0.0,  0.5, -0.8, // Non-facing side
             0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 0.0,  0.8,  0.5,  0.0, // Right side
             0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 5.0, 0.0,  0.8,  0.5,  0.0, // Right side
             0.0, 0.8,  0.0, 0.92, 0.86, 0.76, 2.5, 5.0,  0.8,  0.5,  0.0, // Right side
             0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 5.0, 0.0,  0.0,  0.5,  0.8, // Facing side
            -0.5, 0.0,  0.5, 0.83, 0.70, 0.44, 0.0, 0.0,  0.0,  0.5,  0.8, // Facing side
             0.0, 0.8,  0.0, 0.92, 0.86, 0.76, 2.5, 5.0,  0.0,  0.5,  0.8, // Facing side
        };

    const indices: [18]u32 =
        .{
            0, 1, 2, // Bottom side
            0, 2, 3, // Bottom side
            4, 6, 5, // Left side
            7, 9, 8, // Non-facing side
            10, 12, 11, // Right side
            13, 15, 14, // Facing side
        };

    const lightVertices: [24]f32 =
        .{ 
            -0.1, -0.1,  0.1, 
            -0.1, -0.1, -0.1, 
             0.1, -0.1, -0.1, 
             0.1, -0.1,  0.1, 
            -0.1,  0.1,  0.1, 
            -0.1,  0.1, -0.1, 
             0.1,  0.1, -0.1, 
             0.1,  0.1,  0.1 
        };

    const lightIndices: [36]u32 =
        .{ 
            0, 1, 2, 
            0, 2, 3, 
            0, 4, 7, 
            0, 7, 3, 
            3, 7, 6, 
            3, 6, 2, 
            2, 6, 5, 
            2, 5, 1, 
            1, 5, 4, 
            1, 4, 0, 
            4, 5, 6, 
            4, 6, 7 
        };

    const currentPath = try runtime.utils.path.basePath();

    const vertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources/shaders/default.vert" });
    defer allocator.free(vertexFile);

    const fragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources/shaders/default.frag" });
    defer allocator.free(fragmentFile);

    var shader = try gl.Shader.init(vertexFile, fragmentFile);
    defer shader.deinit();

    var vao = gl.VertexArray.init();
    defer vao.deinit();
    vao.bind();

    var vbo = gl.VertexBuffer.init(&vertices);
    defer vbo.deinit();
    vbo.bind();

    var ebo = gl.ElementBuffer.init(&indices, @sizeOf(u32) * indices.len);
    defer ebo.deinit();
    ebo.bind();

    vao.enableAttrib(&vbo, 0, 3, .float, @sizeOf(f32) * 11, 0);
    vao.enableAttrib(&vbo, 1, 3, .float, @sizeOf(f32) * 11, 3 * @sizeOf(f32));
    vao.enableAttrib(&vbo, 2, 2, .float, @sizeOf(f32) * 11, 6 * @sizeOf(f32));
    vao.enableAttrib(&vbo, 3, 3, .float, @sizeOf(f32) * 11, 8 * @sizeOf(f32));

    vao.unbind();
    vbo.unbind();
    ebo.unbind();

    const lightVertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources/shaders/light.vert" });
    defer allocator.free(lightVertexFile);

    const lightFragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources/shaders/light.frag" });
    defer allocator.free(lightFragmentFile);

    var lightShader = try gl.Shader.init(lightVertexFile, lightFragmentFile);
    defer lightShader.deinit();

    var lightVao = gl.VertexArray.init();
    defer lightVao.deinit();
    lightVao.bind();

    var lightVbo = gl.VertexBuffer.init(&lightVertices);
    defer lightVbo.deinit();
    lightVbo.bind();

    var lightEbo = gl.ElementBuffer.init(&lightIndices, @sizeOf(u32) * lightIndices.len);
    defer lightEbo.deinit();
    lightEbo.bind();

    lightVao.enableAttrib(&lightVbo, 0, 3, .float, @sizeOf(f32) * 3, 0);

    lightVao.unbind();
    lightVbo.unbind();
    lightEbo.unbind();

    const lightColor: za.Vec4 = za.Vec4.set(1.0);
    const lightPos: za.Vec3 = za.Vec3.set(0.5);
    var lightModel: za.Mat4 = za.Mat4.identity();
    lightModel = lightModel.translate(lightPos);

    const pyramidPos: za.Vec3 = za.Vec3.set(0.0);
    var pyramidModel: za.Mat4 = za.Mat4.identity();
    pyramidModel = pyramidModel.translate(pyramidPos);

    lightShader.use();
    zgl.uniformMatrix4fv(zgl.getUniformLocation(lightShader.id, "model"), false, &.{lightModel.data});
    zgl.uniform4f(zgl.getUniformLocation(lightShader.id, "lightColor"), lightColor.x(), lightColor.y(), lightColor.z(), lightColor.w());
    
    shader.use();
    zgl.uniformMatrix4fv(zgl.getUniformLocation(shader.id, "model"), false, &.{pyramidModel.data});
    zgl.uniform4f(zgl.getUniformLocation(shader.id, "lightColor"), lightColor.x(), lightColor.y(), lightColor.z(), lightColor.w());
    zgl.uniform3f(zgl.getUniformLocation(shader.id, "lightPos"), lightPos.x(), lightPos.y(), lightPos.z());

    const albedoFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources/textures/brick.png" });
    defer allocator.free(albedoFile);
    var tex = try gl.Texture.init(albedoFile, .@"2d", .texture_0, .rgba, .unsigned_byte);
    defer tex.deinit();

    tex.texUnit(&shader, "tex0", 0);

    // Position camera to view the pyramid: slightly back and up
    // Camera position: (0, 0.5, 3.0) looking towards origin (0, 0, 0)
    var camera = gl.Camera.init(za.Vec3.new(0.0, 0.0, 2.0));

    var context = RenderContext{ 
        .shader = &shader, 
        .vao = &vao, 
        .ebo = &ebo, 
        .tex = &tex, 
        .lightShader = &lightShader, 
        .lightVao = &lightVao, 
        .lightEbo = &lightEbo, 
        .cam = &camera 
    };
    runtime.render.setCallback(@ptrCast(&onRender), @ptrCast(&context));
    runtime.event.setCallback(@ptrCast(&onEvent), @ptrCast(&context));

    while (!shouldClose) {
        runtime.render.pool(false);
    }


}

test "runa test" {
    std.debug.print("Hello world!\n");
}
