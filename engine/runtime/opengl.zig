const std = @import("std");
const runtime = @import("runtime.zig");
const allocator = runtime.defaultAllocator();
const io = runtime.io;
const logs = runtime.utils.logs;
const render = &runtime.render;
const utils = @import("utils.zig");
const sdl = @import("sdl3");
const zgl = @import("zgl");
const za = @import("zalgebra");

pub const Driver = enum(u8) { core = 0, es = 1 };

pub const Backend = struct {
    window: sdl.video.Window,
    context: sdl.video.gl.Context,
    glslVersion: [:0]const u8,

    pub fn init(driver: Driver) !Backend {
        var self: Backend = undefined;

        const wasInit = sdl.wasInit(.{ .video = true });
        if (wasInit.video == true) {
            logs.err("Video subsystem has already been initilized", .{});
            return sdl.errors.Error.SdlError;
        }
        
        sdl.init(.{ .video = true }) catch {
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        };

        switch (driver) {
            .core => {
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_profile_mask, @intFromEnum(sdl.video.gl.Profile.core)) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_major_version, 4) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_minor_version, 6) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                self.glslVersion = "#version 460 core";
            },
            .es => {
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_profile_mask, @intFromEnum(sdl.video.gl.Profile.es)) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_major_version, 3) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                sdl.video.gl.setAttribute(sdl.video.gl.Attribute.context_minor_version, 2) catch {
                    logs.sdlErr();
                    return sdl.errors.Error.SdlError;
                };
                self.glslVersion = "#version 320 es";
            },
        }

        self.window = sdl.video.Window.init("Runa", 1024, 576, .{.resizable = true, .open_gl = true}) catch {
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        }; 

        self.context = sdl.video.gl.Context.init(self.window) catch {
            self.window.deinit();
            logs.sdlErr();
            return sdl.errors.Error.SdlError;
        };

        try zgl.loadExtensions({}, Backend.loader);

        zgl.enable(.depth_test);

        return self;
    }

    pub fn deinit(self: *Backend) void {
        self.context.deinit() catch {
            logs.sdlErr();
        };
        self.window.deinit();
        self.glslVersion = "";
        sdl.quit(.{ .video = true });
    }

    fn loader(_: void, name: [:0]const u8) ?*const anyopaque {
        return sdl.video.gl.getProcAddress(name);
    }
};

pub const Render = struct {
    backend: Backend,
    callback: ?*const fn (ctx: ?*anyopaque, delta: f64) void,
    context: ?*anyopaque,

    pub fn init() Render {
        var self: Render = undefined;
        self.backend = undefined;
        self.callback = null;
        self.context = null;

        return self;
    }

    pub fn deinit(self: *Render) void {
        self.backend.deinit();
        self.callback = null;
        self.context = null;
    }

    pub fn initBackend(self: *Render, driver: Driver) !void {
        self.backend = try Backend.init(driver);
    }

    pub fn deinitBackend(self: *Render) void {
        self.backend.deinit();
    }

    pub fn setCallback(self: *Render, cb: *const fn (ctx: ?*anyopaque, delta: f64) void, ctx: ?*anyopaque) void {
        self.callback = cb;
        self.context = ctx;
    }

    pub fn pool(self: *Render, wait: bool) void {
        const time = &runtime.time;
        time.updateCurrentTime();

        const shouldLimitFPS: bool = runtime.gameUserSettings.getFramerateLimit() > 0 and runtime.gameUserSettings.getVsync() == .immediate;
        var frameTime: u64 = 0;
        if (shouldLimitFPS) {
            frameTime = 1000000000 / @as(u64, runtime.gameUserSettings.getFramerateLimit());
        }

        runtime.event.run(wait);

        zgl.clearColor(0.07, 0.13, 0.17, 1.0);
        zgl.clear(.{ .color = true, .depth = true });

        if (self.callback) |cb| cb(self.context, time.delta());

        sdl.video.gl.swapWindow(self.backend.window) catch {
            logs.sdlErr();
        };

        if (shouldLimitFPS) {
            if (frameTime > 0 and frameTime > time.elapsedNS()) {
                sdl.timer.delayNanoseconds(frameTime - time.elapsedNS());
            }
        }

        time.updateEndTime();
    }
};

pub const Mesh = struct {
    vertices: []Vertex,
    indices: []u32,
    textures: []Texture,
    vao: VertexArray,

    pub fn init(vertices: []const Vertex, indices: []const u32, textures: []const Texture) !Mesh {
        var self: Mesh = undefined;
        self.vertices = allocator.dupe(Vertex, vertices) catch |err| {
            logs.err("Mesh init error: {s}", .{@errorName(err)});
            return err;
        };
        self.indices = allocator.dupe(u32, indices) catch |err| {
            allocator.free(self.vertices);
            logs.err("Mesh init error: {s}", .{@errorName(err)});
            return err;
        };
        self.textures = allocator.dupe(Texture, textures) catch |err| {
            allocator.free(self.vertices);
            allocator.free(self.indices);
            logs.err("Mesh init error: {s}", .{@errorName(err)});
            return err;
        };

        self.vao = VertexArray.init();
        self.vao.bind();
        var vbo = VertexBuffer.init(vertices);
        defer vbo.deinit();
        var ebo = ElementBuffer.init(indices);
        defer ebo.deinit();

        self.vao.enableAttrib(&vbo, 0, 3, .float, @sizeOf(Vertex), 0);
        self.vao.enableAttrib(&vbo, 1, 3, .float, @sizeOf(Vertex), 3 * @sizeOf(f32));
        self.vao.enableAttrib(&vbo, 2, 3, .float, @sizeOf(Vertex), 6 * @sizeOf(f32));
        self.vao.enableAttrib(&vbo, 3, 2, .float, @sizeOf(Vertex), 9 * @sizeOf(f32));

        self.vao.unbind();
        vbo.unbind();
        ebo.unbind();

        return self;
    }

    pub fn deinit(self: *Mesh) void {
        allocator.free(self.vertices);
        allocator.free(self.indices);
        for (self.textures) |*texture| {
            texture.deinit();
        }
        allocator.free(self.textures);
        self.vao.deinit();
    }

    pub fn draw(self: *Mesh, shader: *Shader, camera: *Camera) void {
        shader.use();
        self.vao.bind();

        var numDiffuse: u32 = 0;
        var numSpecular: u32 = 0;

        for (self.textures, 0..) |texture, i| {
            var num: []u8 = undefined;
            const textype = texture.texType;

            var numBuf: [126:0]u8 = undefined;
            var uniformBuf: [128:0]u8 = undefined;
            if (std.mem.eql(u8, texture.texType, "diffuse")) {
                numDiffuse += 1;
                num = std.fmt.bufPrintZ(&numBuf, "{}", .{numDiffuse}) catch |err| {
                    logs.err("Mesh draw error: {s}", .{@errorName(err)});
                    return;
                };
            } else if (std.mem.eql(u8, texture.texType, "specular")) {
                numSpecular += 1;
                num = std.fmt.bufPrintZ(&numBuf, "{}", .{numSpecular}) catch |err| {
                    logs.err("Mesh draw error: {s}", .{@errorName(err)});
                    return;
                };
            }
            const uniform = std.fmt.bufPrintZ(&uniformBuf, "{s}{s}", .{textype, num}) catch |err| {
                logs.err("Mesh draw error: {s}", .{@errorName(err)});
                return;
            };
            texture.texUnit(shader, uniform, @intCast(i));
            texture.bind();
        }

        zgl.uniform3f(zgl.getUniformLocation(shader.id, "camPos"), camera.position.x(), camera.position.y(), camera.position.z());
        camera.matrix(shader, "camMatrix");

        zgl.drawElements(.triangles, self.indices.len, .unsigned_int, 0);
    }
};

pub const Camera = struct {
    position: za.Vec3,
    orientation: za.Vec3,
    direction: za.Vec3,
    camMatrix: za.Mat4,
    speed: f32,
    sensitivity: f32,

    pub fn init(position: za.Vec3) Camera {
        var self: Camera = undefined;
        self.position = position;
        self.orientation = za.Vec3.new(0.0, 0.0, -1.0);
        self.direction = za.Vec3.zero();
        self.camMatrix = za.Mat4.identity();
        self.sensitivity = 120.0;
        self.speed = 2.0;
        return self;
    }

    pub fn editorInput(self: *Camera, event: sdl.events.Event) void {
        var input = &runtime.inputSystem;
        const inputDir: za.Vec2 = input.vector(.a, .d, .w, .s);
        self.direction = za.Vec3.norm(
            za.Vec3.cross(self.orientation, za.Vec3.up()))
            .mul(za.Vec3.set(inputDir.x()))
            .add(za.Vec3.norm(self.orientation)
            .mul(za.Vec3.set(inputDir.y()))
        );

        self.direction = za.Vec3.add(
            za.Vec3.mul(za.Vec3.norm(za.Vec3.cross(self.orientation, za.Vec3.up())), za.Vec3.set(-inputDir.x())),
            za.Vec3.mul(za.Vec3.norm(self.orientation), za.Vec3.set(inputDir.y())),
        );

        const yAxis = input.axis(.space, .left_ctrl);
        self.direction.yMut().* = yAxis;
        self.speed = if (input.keyPressed(.left_shift)) 6.0 else 2.0;

        if (input.mouseButtonPressed(.right)) {
            const windowSize = render.backend.window.getSize() catch return;
            sdl.mouse.setWindowGrab(render.backend.window, true) catch {
                logs.sdlErr();
            };
            sdl.mouse.setWindowRelativeMode(render.backend.window, true) catch {
                logs.sdlErr();
            };
            sdl.mouse.hide() catch {
                logs.sdlErr();
            };

            switch (event) {
                .mouse_motion => |e| {
                    const xrel: f32 = e.x_rel;
                    const yrel: f32 = e.y_rel;

                    const rotX: f32 = self.sensitivity * yrel / @as(f32, @floatFromInt(windowSize.@"1"));
                    const rotY: f32 = self.sensitivity * xrel / @as(f32, @floatFromInt(windowSize.@"0"));

                    // newOrientation = rotate(orientation, -rotX, normalize(cross(orientation, up)))
                    const right = za.Vec3.norm(za.Vec3.cross(self.orientation, za.Vec3.up()));
                    const qx = za.Quat.fromAxis(-rotX, right);
                    const newOrientation = qx.rotateVec(self.orientation);

                    // checa ângulo
                    if (@abs(newOrientation.getAngle(za.Vec3.up()) - 90.0) <= 85.0) {
                        self.orientation = newOrientation;
                    }

                    // orientation = rotate(orientation, -rotY, up)
                    const qy = za.Quat.fromAxis(-rotY, za.Vec3.up());
                    self.orientation = qy.rotateVec(self.orientation);
                },
                else => {}
            }
        } else {
            sdl.mouse.setWindowGrab(render.backend.window, false) catch {
                logs.sdlErr();
            };
            sdl.mouse.setWindowRelativeMode(render.backend.window, false) catch {
                logs.sdlErr();
            };
            sdl.mouse.show() catch {
                logs.sdlErr();
            };
        }
    }

    pub fn editorTick(self: *Camera) void {
        var time = &runtime.time;
        self.direction = za.Vec3.new(
            std.math.clamp(self.direction.x(), -1.0, 1.0), 
            std.math.clamp(self.direction.y(), -1.0, 1.0), 
            std.math.clamp(self.direction.z(), -1.0, 1.0)
        );
        const dt_f32: f32 = @as(f32, @floatCast(time.delta()));
        self.position = self.position.add(
            self.direction.mul(za.Vec3.set(self.speed))
            .mul(za.Vec3.set(dt_f32))
        );
    }

    pub fn updateMatrix(self: *Camera, fov: f32, nearPlane: f32, farPlane: f32) void {
        const windowSize = render.backend.window.getSize() catch return;

        var view: za.Mat4 = za.Mat4.identity();
        var projection: za.Mat4 = za.Mat4.identity();

        // Makes camera look in the right direction from the right position
        view = za.lookAt(self.position, self.position.add(self.orientation), za.Vec3.up());
        projection = za.perspective(fov, @as(f32, @floatFromInt(windowSize.@"0")) / @as(f32, @floatFromInt(windowSize.@"1")), nearPlane, farPlane);
        
        self.camMatrix = projection.mul(view);
    }

    pub fn matrix(self: *Camera, shader: *Shader, uniform: [:0]const u8) void {
        zgl.uniformMatrix4fv(zgl.getUniformLocation(shader.id, uniform), false, &.{self.camMatrix.data});
    }
};

pub const Shader = struct {
    id: zgl.Program,

    pub fn init(vertexFile: []const u8, fragmentFile: []const u8) !Shader {
        var self: Shader = undefined;
        // Convert the shader source strings into character arrays
        const vertexSource: []u8 = try io.readFile(vertexFile, .read_text);
        defer allocator.free(vertexSource);

        const fragmentSource: []u8 = try io.readFile(fragmentFile, .read_text);
        defer allocator.free(fragmentSource);

        // Create Vertex Shader Object and get its reference
        var vertexShader = zgl.createShader(.vertex);
        // Attach Vertex Shader source to the Vertex Shader Object
        vertexShader.source(1, &.{vertexSource});
        // Compile the Vertex Shader into machine code
        vertexShader.compile();
        // Checks if Shader compiled succesfully
	    try Shader.shaderCompLog(vertexShader, .vertex);

        // Create Fragment Shader Object and get its reference
        var fragmentShader = zgl.createShader(.fragment);
        // Attach Fragment Shader source to the Fragment Shader Object
        fragmentShader.source(1, &.{fragmentSource});
        // Compile the Vertex Shader into machine code
        fragmentShader.compile();
        // Checks if Shader compiled succesfully
	    try Shader.shaderCompLog(vertexShader, .fragment);

        // Create Shader Program Object and get its reference
        self.id = zgl.createProgram();
        // Attach the Vertex and Fragment Shaders to the Shader Program
        self.id.attach(vertexShader);
        self.id.attach(fragmentShader);
        // Wrap-up/Link all the shaders together into the Shader Program
        self.id.link();
        // Checks if Shader compiled succesfully
	    try Shader.shaderCompLog(vertexShader, .compute);

        // Delete the now useless Vertex and Fragment Shader objects
        vertexShader.delete();
        fragmentShader.delete();

        return self;
    }

    pub fn deinit(self: *Shader) void {
        self.id.delete();
        self.id = .invalid;
    }

    pub fn use(self: *Shader) void {
        self.id.use();
    }

    pub fn setUniformLocation(self: *const Shader, uniform: [:0]const u8, unit: u32) void {
        // Gets the location of the uniform
        const textuni = zgl.getUniformLocation(self.id, uniform);
        // Shader needs to be activated before changing the value of a uniform
        self.use();
        // Sets the value of the uniform
        zgl.uniform1i(textuni, unit);
    }

    fn shaderCompLog(shader: zgl.Shader, shaderType: zgl.ShaderType) !void {
        // Stores status of compilation
        const hasCompiled: i32 = zgl.getShader(shader, .compile_status);

        if (hasCompiled == 0)
        {
            const infoLog = try zgl.getShaderInfoLog(shader, allocator);
            defer allocator.free(infoLog);
            logs.err("SHADER_COMPILATION_ERROR - {any}: {s}\n", .{shaderType, infoLog});
        }
    }

    fn programCompLog(shader: zgl.Program, shaderType: zgl.ShaderType) !void {
        // Stores status of compilation
        const hasCompiled: i32 = zgl.getProgram(shader, .link_status);

        if (hasCompiled == 0)
        {
            const infoLog = try zgl.getShaderInfoLog(shader, allocator);
            defer allocator.free(infoLog);
            logs.err("PROGRAM_COMPILATION_ERROR - {any}: {s}\n", .{shaderType, infoLog});
        }
    }
};

pub const Texture = struct {
    id: zgl.Texture,
    texType: []const u8,
    unit: u32,

    pub fn init(textureFile: []const u8, textype: []const u8, slot: u32, format: zgl.PixelFormat, pixeltype: zgl.PixelType) !Texture {
        var self: Texture = undefined;
        self.texType = textype;
        
        const dupezPath = allocator.dupeZ(u8, textureFile) catch |err| {
            logs.err("{any}", .{@errorName(err)});
            return err;
        };

        const surface = sdl.image.loadFile(dupezPath) catch |err| {
            logs.sdlErr();
            return err;
        };
        // Deletes the image data as it is already in the OpenGL Texture object
        defer surface.deinit();

        allocator.free(dupezPath);

        const pixels = surface.getPixels() orelse return error.TextureLoadFailed;

        // Generates an OpenGL texture object
        self.id = zgl.genTexture();
        // Assigns the texture to a Texture Unit
        zgl.binding.activeTexture(@as(u32, @intFromEnum(zgl.TextureUnit.texture_0) + slot));
        self.unit = slot;
        zgl.bindTexture(self.id , .@"2d");

        // Configures the type of algorithm that is used to make the image smaller or bigger
        zgl.texParameter(.@"2d", .min_filter, .nearest_mipmap_linear);
        zgl.texParameter(.@"2d", .mag_filter, .nearest);

        // Configures the way the texture repeats (if it does at all)
        zgl.texParameter(.@"2d", .wrap_r, .repeat);
        zgl.texParameter(.@"2d", .wrap_t, .repeat);

        // Extra lines in case you choose to use GL_CLAMP_TO_BORDER
        // float flatColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, flatColor);

        // Assigns the image to the OpenGL Texture object
        zgl.textureImage2D(.@"2d", 0, .rgba, surface.getWidth(), surface.getHeight(), format, pixeltype, pixels.ptr);
        // Generates MipMaps
        zgl.generateMipmap(.@"2d");

        // Unbinds the OpenGL Texture object so that it can't accidentally be modified
        zgl.bindTexture(.invalid, .@"2d");

        self.texType = textype;
        return self;
    }

    pub fn deinit(self: *Texture) void {
        self.id.delete();
    }

    pub fn texUnit(self: *const Texture, shader: *Shader, uniform: [:0]const u8, unit: i32) void {
        _ = self;
        // Gets the location of the uniform
        const texUniform = zgl.getUniformLocation(shader.id, uniform);
        // Shader needs to be activated before changing the value of a uniform
        shader.use();
        // Sets the value of the uniform
        zgl.uniform1i(texUniform, unit);
    }

    pub fn bind(self: *const Texture) void {
        zgl.binding.activeTexture(@as(u32, @intFromEnum(zgl.TextureUnit.texture_0) + self.unit));
        zgl.bindTexture(self.id, .@"2d");
    }

    pub fn unbind(self: *const Texture) void {
        _ = self;
        zgl.bindTexture(.invalid, .@"2d");
    }
};

pub const ElementBuffer = struct {
    id: zgl.Buffer,
    len: usize,
    size: usize,

    pub fn init(indices: []const u32) ElementBuffer {
        var self: ElementBuffer = undefined;
        self.id = zgl.genBuffer();
        zgl.bindBuffer(self.id, .element_array_buffer);
        zgl.bufferData(.element_array_buffer, u32, indices, .static_draw);
        self.len = indices.len;
        self.size = indices.len * @sizeOf(u32);
        return self;
    }

    pub fn deinit(self: *ElementBuffer) void {
        self.id.delete();
    }

    pub fn bind(self: *const ElementBuffer) void {
        zgl.bindBuffer(self.id, .element_array_buffer);
    }

    pub fn unbind(self: *const ElementBuffer) void {
        _ = self;
        zgl.bindBuffer(.invalid, .element_array_buffer);
    }
};

pub const Vertex = struct {
    data: [11]f32,

    pub fn init(position: za.Vec3, normal: za.Vec3, color: za.Vec3, uv: za.Vec2) Vertex {
        var self: Vertex = undefined;
        self.data = .{position.x(), position.y(), position.z(), normal.x(), normal.y(), normal.z(), color.x(), color.y(), color.z(), uv.x(), uv.y()};
        return self;
    }

};

pub const VertexBuffer = struct {
    id: zgl.Buffer,

    pub fn init(vertices: []const Vertex) VertexBuffer {
        var self: VertexBuffer = undefined;
        self.id = zgl.genBuffer();
        zgl.bindBuffer(self.id, .array_buffer);
        zgl.bufferData(.array_buffer, Vertex, vertices, .static_draw);
        return self;
    }

    pub fn deinit(self: *VertexBuffer) void {
        self.id.delete();
        self.id = .invalid;
    }

    pub fn bind(self: *const VertexBuffer) void {
        zgl.bindBuffer(self.id, .array_buffer);
    }

    pub fn unbind(self: *const VertexBuffer) void {
        _ = self;
        zgl.bindBuffer(.invalid, .array_buffer);
    }
};

pub const VertexArray = struct {
    id: zgl.VertexArray,

    pub fn init() VertexArray {
        var self: VertexArray = undefined;
        self.id = zgl.genVertexArray();

        return self;
    }

    pub fn deinit(self: *VertexArray) void {
        self.id.delete();
        self.id = .invalid;
    }

    pub fn bind(self: *const VertexArray) void {
        zgl.bindVertexArray(self.id);
    }

    pub fn unbind(self: *const VertexArray) void {
        _ = self;
        zgl.bindVertexArray(.invalid);
    }

    pub fn enableAttrib(self: *const VertexArray, vertexBuf: *VertexBuffer, layout: u32, num: u32, attribType: zgl.Type, stride: usize, offset: usize) void {
        _ = self;
        vertexBuf.bind();
        zgl.vertexAttribPointer(layout, num, attribType, false, stride, offset);
        zgl.enableVertexAttribArray(layout);
        vertexBuf.unbind();
    }
};
