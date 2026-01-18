const std = @import("std");
const runtime = @import("runtime");
const allocator = runtime.defaultAllocator();
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
    if (@as(?*RenderContext, @ptrCast(@alignCast(ctx)))) |renderContext| {
        context = renderContext;
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
    floorMesh: *gl.Mesh,
    lightShader: *gl.Shader, 
    lightMesh: *gl.Mesh,
    cam: *gl.Camera 
};

fn onRender(ctx: ?*anyopaque, delta: f64) void {
    _ = delta;

    var context: *RenderContext = undefined;
    if (@as(?*RenderContext, @ptrCast(@alignCast(ctx)))) |renderContext| {
        context = renderContext;
    } else {
        return;
    }

    context.cam.editorTick();
    context.cam.updateMatrix(80.0, 0.1, 100);

    context.floorMesh.draw(context.shader, context.cam);
    context.lightMesh.draw(context.lightShader, context.cam);
}

pub fn main() !void {
    try runtime.render.initBackend(.core);

    defer {
        runtime.inputSystem.deinit();
        runtime.render.deinit();
    }

    try runtime.gameUserSettings.setVsync(.immediate);
    //runtime.gameUserSettings.setFramerateLimit(300);

    const vertices = [_]gl.Vertex{
        gl.Vertex.init(za.Vec3.new(-1.0, 0.0,  1.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec2.new(0.0, 0.0)),
        gl.Vertex.init(za.Vec3.new(-1.0, 0.0, -1.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec2.new(0.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 1.0, 0.0, -1.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec2.new(1.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 1.0, 0.0,  1.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec3.new(0.0, 1.0, 0.0), za.Vec2.new(1.0, 0.0))
    };


    const indices = [_]u32{
        0, 1, 2,
        0, 2, 3
    };

    const lightVertices = [_]gl.Vertex{
        gl.Vertex.init(za.Vec3.new(-0.1, -0.1,  0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(0.0, 0.0)),
        gl.Vertex.init(za.Vec3.new(-0.1, -0.1, -0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(0.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 0.1, -0.1, -0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(1.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 0.1, -0.1,  0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(1.0, 0.0)),
        gl.Vertex.init(za.Vec3.new(-0.1,  0.1,  0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(0.0, 0.0)),
        gl.Vertex.init(za.Vec3.new(-0.1,  0.1, -0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(0.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 0.1,  0.1, -0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(1.0, 1.0)),
        gl.Vertex.init(za.Vec3.new( 0.1,  0.1,  0.1), za.Vec3.new(0.0, 0.0, 0.0), za.Vec3.new(1.0, 1.0, 1.0), za.Vec2.new(1.0, 0.0))
    };

    const lightIndices = [_]u32{
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
    defer allocator.free(currentPath);

    const albedoFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/textures/planks.png" });
    defer allocator.free(albedoFile);

    const specularFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/textures/planksSpec.png" });
    defer allocator.free(specularFile);
    
    // Texture data
    const textures = [_]gl.Texture{
        try gl.Texture.init(albedoFile, "diffuse", 0, .rgba, .unsigned_byte),
        try gl.Texture.init(specularFile, "specular", 1, .red, .unsigned_byte)
    };

    const vertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/shaders/default.vert" });
    defer allocator.free(vertexFile);

    // Generates Shader object using shaders default.vert and default.frag
    const fragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/shaders/default.frag" });
    defer allocator.free(fragmentFile);

    var shader = try gl.Shader.init(vertexFile, fragmentFile);
    defer shader.deinit(); 
    // Create floor mesh
    var floor = try gl.Mesh.init(&vertices, &indices, &textures);
    defer floor.deinit();

    // Shader for light cube
    const lightVertexFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/shaders/light.vert" });
    defer allocator.free(lightVertexFile);

    const lightFragmentFile = try std.mem.concat(allocator, u8, &.{ currentPath, "/resources/shaders/light.frag" });
    defer allocator.free(lightFragmentFile);

    var lightShader = try gl.Shader.init(lightVertexFile, lightFragmentFile);
    defer lightShader.deinit();
    
    // Create light cube mesh
    var light = try gl.Mesh.init(&lightVertices, &lightIndices, &textures);
    defer light.deinit();
    

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
    // Position camera to view the pyramid: slightly back and up
    // Camera position: (0, 0.5, 3.0) looking towards origin (0, 0, 0)
    var camera = gl.Camera.init(za.Vec3.new(0.0, 0.0, 2.0));

    var context = RenderContext{ 
        .shader = &shader, 
        .floorMesh = &floor,
        .lightShader = &lightShader, 
        .lightMesh = &light,
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
