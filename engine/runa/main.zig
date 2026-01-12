const std = @import("std");
const runtime = @import("runtime");
const String = @import("string").String;
const gl = runtime.gl;
const za = @import("zalgebra");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("glad/glad.h");
});

var shouldClose = false;

fn onEvent(e: c.SDL_Event, ctx: ?*anyopaque) void {
    _ = ctx;
    //const context: ?*RenderContext = @ptrCast(@alignCast(ctx));

    if (e.type == c.SDL_EVENT_QUIT) {
        shouldClose = true;
    }

    var w: i32 = 0;
    var h: i32 = 0;
    if (!c.SDL_GetWindowSizeInPixels(@ptrCast(runtime.render.backend.window), &w, &h)) {
        runtime.log.sdlErr();
    }

    if (e.type == c.SDL_EVENT_WINDOW_RESIZED) {
        c.glViewport(0, 0, w, h);
    }

    //if (context) |contxt| {
    //    contxt.cam.editorInput(@ptrCast(&e), w, h);
    //}
}

const RenderContext = struct { shader: *gl.Shader, vao: *gl.VertexArray, ebo: *gl.ElementBuffer, tex: *gl.Texture, lightShader: *gl.Shader, lightVao: *gl.VertexArray, lightEbo: *gl.ElementBuffer, cam: *gl.Camera };

fn onRender(ctx: ?*anyopaque, delta: f64) void {
    _ = delta;
    const context: ?*RenderContext = @ptrCast(@alignCast(ctx));

    if (context) |contxt| {
        //contxt.cam.editorTick();
        contxt.cam.updateMatrix(60.0, 0.1, 100);

        contxt.shader.use();
        c.glUniform3f(c.glGetUniformLocation(contxt.shader.id, "camPos"), contxt.cam.position.x(), contxt.cam.position.y(), contxt.cam.position.z());
        contxt.cam.matrix(contxt.shader, "camMatrix");
        contxt.tex.bind();
        contxt.vao.bind();
        c.glDrawElements(c.GL_TRIANGLES, @as(i32, @intCast(@divTrunc(contxt.ebo.size, @sizeOf(u32)))), c.GL_UNSIGNED_INT, null);

        contxt.lightShader.use();
        contxt.cam.matrix(contxt.lightShader, "camMatrix");
        contxt.lightVao.bind();
        c.glDrawElements(c.GL_TRIANGLES, @as(i32, @intCast(@divTrunc(contxt.lightEbo.size, @sizeOf(u32)))), c.GL_UNSIGNED_INT, null);
    }
}

pub fn main() !void {
    const allocator = runtime.defaultAllocator();
    if (!runtime.render.initBackend(.core))
        return;

    defer runtime.render.deinitBackend();

    //runtime.gameUserSettings.setVsync(.disable);
    //runtime.gameUserSettings.setFramerateLimit(300);

    const vertices: [176]f32 =
        .{
            -0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 0.0, 0.0, 0.0, -1.0, 0.0, // Bottom side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 5.0, 0.0, -1.0, 0.0, // Bottom side
            0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 5.0, 0.0, -1.0, 0.0, // Bottom side
            0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 5.0, 0.0, 0.0, -1.0, 0.0, // Bottom side
            -0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 0.0, 0.0, -0.8, 0.5, 0.0, // Left Side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 0.0, -0.8, 0.5, 0.0, // Left Side
            0.0, 0.8, 0.0, 0.92, 0.86, 0.76, 2.5, 5.0, -0.8, 0.5, 0.0, // Left Side
            -0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 5.0, 0.0, 0.0, 0.5, -0.8, // Non-facing side
            0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 0.0, 0.0, 0.5, -0.8, // Non-facing side
            0.0, 0.8, 0.0, 0.92, 0.86, 0.76, 2.5, 5.0, 0.0, 0.5, -0.8, // Non-facing side
            0.5, 0.0, -0.5, 0.83, 0.70, 0.44, 0.0, 0.0, 0.8, 0.5, 0.0, // Right side
            0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 5.0, 0.0, 0.8, 0.5, 0.0, // Right side
            0.0, 0.8, 0.0, 0.92, 0.86, 0.76, 2.5, 5.0, 0.8, 0.5, 0.0, // Right side
            0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 5.0, 0.0, 0.0, 0.5, 0.8, // Facing side
            -0.5, 0.0, 0.5, 0.83, 0.70, 0.44, 0.0, 0.0, 0.0, 0.5, 0.8, // Facing side
            0.0, 0.8, 0.0, 0.92, 0.86, 0.76, 2.5, 5.0, 0.0, 0.5, 0.8, // Facing side
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
        .{ -0.1, -0.1, 0.1, -0.1, -0.1, -0.1, 0.1, -0.1, -0.1, 0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, 0.1, 0.1, 0.1 };

    const lightIndices: [36]u32 =
        .{ 0, 1, 2, 0, 2, 3, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 2, 6, 5, 2, 5, 1, 1, 5, 4, 1, 4, 0, 4, 5, 6, 4, 6, 7 };

    const currentPath = runtime.path.currentPath();

    const vertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\default.vert", &[_]u8{0} });
    defer allocator.free(vertexFile);

    const fragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\default.frag", &[_]u8{0} });
    defer allocator.free(fragmentFile);

    const shaderInit = gl.Shader.init(vertexFile, fragmentFile);
    // `vertexFile` and `fragmentFile` are freed by the defers above
    if (!shaderInit.status) {
        return;
    }
    var shader = shaderInit.shader;
    defer shader.deinit();

    var vao = gl.VertexArray.init();
    defer vao.deinit();
    vao.bind();

    var vbo = gl.VertexBuffer.init(&vertices, @sizeOf(f32) * vertices.len);
    defer vbo.deinit();
    vbo.bind();

    var ebo = gl.ElementBuffer.init(&indices, @sizeOf(u32) * indices.len);
    defer ebo.deinit();
    ebo.bind();

    vao.enableAttrib(&vbo, 0, 3, c.GL_FLOAT, @sizeOf(f32) * 11, @ptrFromInt(0));
    vao.enableAttrib(&vbo, 1, 3, c.GL_FLOAT, @sizeOf(f32) * 11, @ptrFromInt(3 * @sizeOf(f32)));
    vao.enableAttrib(&vbo, 2, 2, c.GL_FLOAT, @sizeOf(f32) * 11, @ptrFromInt(6 * @sizeOf(f32)));
    vao.enableAttrib(&vbo, 3, 3, c.GL_FLOAT, @sizeOf(f32) * 11, @ptrFromInt(8 * @sizeOf(f32)));

    vao.unbind();
    vbo.unbind();
    ebo.unbind();

    const lightVertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\light.vert", &[_]u8{0} });
    defer allocator.free(lightVertexFile);

    const lightFragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\light.frag", &[_]u8{0} });
    defer allocator.free(lightFragmentFile);

    const lightShaderInit = gl.Shader.init(lightVertexFile, lightFragmentFile);
    if (!lightShaderInit.status) {
        return;
    }
    var lightShader = lightShaderInit.shader;
    defer lightShader.deinit();

    var lightVao = gl.VertexArray.init();
    defer lightVao.deinit();
    lightVao.bind();

    var lightVbo = gl.VertexBuffer.init(&lightVertices, @sizeOf(f32) * lightVertices.len);
    defer lightVbo.deinit();
    lightVbo.bind();

    var lightEbo = gl.ElementBuffer.init(&lightIndices, @sizeOf(u32) * lightIndices.len);
    defer lightEbo.deinit();
    lightEbo.bind();

    lightVao.enableAttrib(&lightVbo, 0, 3, c.GL_FLOAT, @sizeOf(f32) * 3, @ptrFromInt(0));

    lightVao.unbind();
    lightVbo.unbind();
    lightEbo.unbind();

    const lightColor: za.Vec4 = za.Vec4.set(1.0);
    const lightPos: za.Vec3 = za.Vec3.set(0.5);
    var lightModel: za.Mat4 = za.Mat4.set(1.0);
    lightModel = lightModel.translate(lightPos);

    const pyramidPos: za.Vec3 = za.Vec3.set(0.0);
    var pyramidModel: za.Mat4 = za.Mat4.set(1.0);
    pyramidModel = pyramidModel.translate(pyramidPos);

    lightShader.use();
    c.glUniformMatrix4fv(c.glGetUniformLocation(lightShader.id, "model"), 1, c.GL_FALSE, &lightModel.data[0][0]);
    c.glUniform4f(c.glGetUniformLocation(lightShader.id, "lightColor"), lightColor.x(), lightColor.y(), lightColor.z(), lightColor.w());

    shader.use();
    c.glUniformMatrix4fv(c.glGetUniformLocation(shader.id, "model"), 1, c.GL_FALSE, &pyramidModel.data[0][0]);
    c.glUniform4f(c.glGetUniformLocation(shader.id, "lightColor"), lightColor.x(), lightColor.y(), lightColor.z(), lightColor.w());
    c.glUniform3f(c.glGetUniformLocation(shader.id, "lightPos"), lightPos.x(), lightPos.y(), lightPos.z());


    const albedoFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\textures\\brick.png", &[_]u8{0} });
    defer allocator.free(albedoFile);
    const texInit = gl.Texture.init(albedoFile, c.GL_TEXTURE_2D, c.GL_TEXTURE0, c.GL_RGBA, c.GL_UNSIGNED_BYTE);
    if (!texInit.status) {
        return;
    }
    var tex = texInit.texture;
    defer tex.deinit();

    tex.texUnit(&shader, "tex0", 0);

    // Enables the Depth Buffer
    c.glEnable(c.GL_DEPTH_TEST);

    // Position camera to view the pyramid: slightly back and up
    // Camera position: (0, 0.5, 3.0) looking towards origin (0, 0, 0)
    var camera = gl.Camera.init(za.Vec3.new(0.0, 0.0, 2.0));

    var context = RenderContext{ .shader = &shader, .vao = &vao, .ebo = &ebo, .tex = &tex, .lightShader = &lightShader, .lightVao = &lightVao, .lightEbo = &lightEbo, .cam = &camera };
    runtime.render.setCallback(@ptrCast(&onRender), @ptrCast(&context));
    runtime.event.setCallback(@ptrCast(&onEvent), @ptrCast(&context));

    while (!shouldClose) {
        runtime.render.pool(.pool);
    }


}

test "runa test" {
    std.debug.print("Hello world!\n");
}
