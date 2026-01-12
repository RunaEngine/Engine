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
    //_ = ctx;
    const context: ?*RenderContext = @ptrCast(@alignCast(ctx));

    var w: i32 = 0;
    var h: i32 = 0;
    if (!c.SDL_GetWindowSizeInPixels(@ptrCast(runtime.render.backend.window), &w, &h)) {
        runtime.log.sdlErr();
    }

    if (context) |contxt| {
        contxt.cam.editorInput(@ptrCast(&e), w, h);
    }

    if (e.type == c.SDL_EVENT_WINDOW_RESIZED) {
        c.glViewport(0, 0, w, h);
    }

    if (e.type == c.SDL_EVENT_QUIT) {
        shouldClose = true;
    }
}

const RenderContext = struct { 
    uniID: i32, 
    shader: *gl.Shader, 
    vao: *gl.VertexArray, 
    ebo: *gl.ElementBuffer, 
    tex: *gl.Texture, 
    lightShader: *gl.Shader, 
    lightVao: *gl.VertexArray, 
    lightEbo: *gl.ElementBuffer, 
    cam: *gl.Camera 
};

fn onRender(ctx: ?*anyopaque, delta: f32) void {
    _ = delta;
    const context: ?*RenderContext = @ptrCast(@alignCast(ctx));

    if (context) |contxt| {
        contxt.shader.use();

        contxt.cam.matrix(60.0, 0.1, 100, contxt.shader, "camMatrix");
        contxt.cam.editorTick();

        c.glUniform1f(contxt.uniID, 0.5);
        contxt.tex.bind();
        contxt.vao.bind();

        c.glDrawElements(c.GL_TRIANGLES, @as(i32, @intCast(@divTrunc(contxt.ebo.size, @sizeOf(u32)))), c.GL_UNSIGNED_INT, null);

        //contxt.lightShader.use();
        //contxt.cam.matrix(60.0, 0.1, 100, contxt.lightShader, "camMatrix");
        //contxt.lightVao.bind();
        //c.glDrawElements(c.GL_TRIANGLES, @as(i32, @intCast(@divTrunc(contxt.lightEbo.size, @sizeOf(u32)))), c.GL_UNSIGNED_INT, null);
    }
}

pub fn main() !void {
    const allocator = runtime.defaultAllocator();
    if (!runtime.render.initBackend(.core))
        return;

    //runtime.gameUserSettings.setVsync(.disable);
    //runtime.gameUserSettings.setFramerateLimit(300);

    const vertices: [40]f32 = .{
        -0.5,  0.0,  0.5,  0.83, 0.70, 0.44, 0.0, 0.0,
        -0.5,  0.0, -0.5,  0.83, 0.70, 0.44, 5.0, 0.0,
         0.5,  0.0, -0.5,  0.83, 0.70, 0.44, 0.0, 0.0,
         0.5,  0.0,  0.5,  0.83, 0.70, 0.44, 5.0, 0.0,
         0.0,  0.8,  0.0,  0.92, 0.86, 0.76, 2.5, 5.0,
    };

    const indices: [18]u32 =
        .{
            0, 1, 2,
            0, 2, 3,
            0, 1, 4,
            1, 2, 4,
            2, 3, 4,
            3, 0, 4,
        };

    const lightVertices: [24]f32 = .{ 
        -0.1, -0.1,  0.1, 
        -0.1, -0.1, -0.1, 
         0.1, -0.1, -0.1, 
         0.1, -0.1,  0.1, 
        -0.1,  0.1,  0.1, 
        -0.1,  0.1, -0.1, 
         0.1,  0.1, -0.1, 
         0.1,  0.1,  0.1 
    };

    const lightIndices: [36]u32 = .{ 
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

    // Position camera to view the pyramid: slightly back and up
    // Camera position: (0, 0.5, 3.0) looking towards origin (0, 0, 0)
    var camera = gl.Camera.init(za.Vec3.new(0.0, 0.5, 2.0));

    const currentPath = runtime.path.currentPath();

    const vertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\default.vert", &[_]u8{0} });
    defer allocator.free(vertexFile);

    const fragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\shaders\\default.frag", &[_]u8{0} });
    defer allocator.free(fragmentFile);

    const shaderInit = gl.Shader.init(vertexFile, fragmentFile);
    allocator.free(vertexFile);
    allocator.free(fragmentFile);
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

    vao.enableAttrib(&vbo, 0, 3, c.GL_FLOAT, @sizeOf(f32) * 8, @ptrFromInt(0));
    vao.enableAttrib(&vbo, 1, 3, c.GL_FLOAT, @sizeOf(f32) * 8, @ptrFromInt(3 * @sizeOf(f32)));
    vao.enableAttrib(&vbo, 2, 2, c.GL_FLOAT, @sizeOf(f32) * 8, @ptrFromInt(6 * @sizeOf(f32)));

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

    //var lightColor = za.Vec4.new(1.0, 1.0, 1.0, 1.0);
    const lightPos = za.Vec3.set(0.5);
    var lightModel = za.Mat4.set(1.0);
    lightModel = za.Mat4.translate(lightModel, lightPos);

    const pyramidPos = za.Vec3.new(0.0, 0.0, 0.0);
    var pyramidModel = za.Mat4.set(1.0);
    pyramidModel = za.Mat4.translate(pyramidModel, pyramidPos);

    lightShader.use();
    c.glUniformMatrix4fv(c.glGetUniformLocation(lightShader.id, "model"), 1, c.GL_FALSE, &lightModel.data[0][0]);

    shader.use();
    c.glUniformMatrix4fv(c.glGetUniformLocation(shader.id, "model"), 1, c.GL_FALSE, &pyramidModel.data[0][0]);

    const uniId: i32 = c.glGetUniformLocation(shader.id, "scale");

    const albedoFile = try std.mem.concat(allocator, u8, &.{ currentPath, "resources\\textures\\brick.png", &[_]u8{0} });
    defer allocator.free(albedoFile);
    const texInit = gl.Texture.init(albedoFile, c.GL_TEXTURE_2D, c.GL_TEXTURE0, c.GL_RGB, c.GL_UNSIGNED_BYTE);
    if (!texInit.status) {
        return;
    }
    var tex = texInit.texture;
    defer tex.deinit();

    shader.setUniformLocation("tex0", 0);

    var context = RenderContext{ 
        .uniID = uniId, 
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
        runtime.render.pool(.pool);
    }
}

test "runa test" {
    std.debug.print("Hello world!\n");
}
