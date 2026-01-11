const std = @import("std");
const runtime = @import("runtime.zig");
const log = runtime.log;
const io = runtime.io;
const za = @import("zalgebra");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("glad/glad.h");
    @cInclude("stb_image.h");
});

pub var GL_ELEMENT_COUNT: i64 = 0;

pub const Driver = enum(u8) { core = 0, es = 1 };

pub const Backend = struct {
    window: ?*c.SDL_Window,
    context: c.SDL_GLContext,
    glslVersion: []const u8,

    pub fn init(driver: Driver) struct { status: bool, backend: Backend } {
        var self: Backend = undefined;

        if (c.SDL_WasInit(c.SDL_INIT_VIDEO) != 0)
            return .{ .status = false, .backend = self };

        if (!c.SDL_InitSubSystem(c.SDL_INIT_VIDEO)) {
            log.sdlErr();
            return .{ .status = false, .backend = self };
        }

        switch (driver) {
            .core => {
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_CORE)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 4)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 6)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                self.glslVersion = "#version 460 core";
            },
            .es => {
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 2)) {
                    log.sdlErr();
                    return .{ .status = false, .backend = self };
                }
                self.glslVersion = "#version 300 es";
            },
        }

        self.window = c.SDL_CreateWindow("Runa", 1024, 576, c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_OPENGL);
        if (self.window == null) {
            log.sdlErr();
            return .{ .status = false, .backend = self };
        }

        // Create a SDL renderer
        self.context = c.SDL_GL_CreateContext(self.window);
        if (self.context == null) {
            c.SDL_DestroyWindow(self.window);
            log.sdlErr();
            return .{ .status = false, .backend = self };
        }

        if (driver == .es) {
            if (c.gladLoadGLES2Loader(@ptrCast(&c.SDL_GL_GetProcAddress)) == 0) {
                log.err("Failed to initialize GLAD.");
                c.SDL_DestroyWindow(self.window);
                if (!c.SDL_GL_DestroyContext(self.context))
                    log.sdlErr();
                return .{ .status = false, .backend = self };
            }
        } else {
            if (c.gladLoadGLLoader(@ptrCast(&c.SDL_GL_GetProcAddress)) == 0) {
                log.err("Failed to initialize GLAD.");
                c.SDL_DestroyWindow(self.window);
                if (!c.SDL_GL_DestroyContext(self.context))
                    log.sdlErr();
                return .{ .status = false, .backend = self };
            }
        }

        return .{ .status = true, .backend = self };
    }

    pub fn deinit(self: *Backend) void {
        if (self.context) |ctx|
            c.SDL_GL_DestroyContext(ctx);
        if (self.window) |window|
            c.SDL_DestroyWindow(window);
        self.glslVersion = "";
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

    pub fn initBackend(self: *Render, driver: Driver) bool {
        const bend = Backend.init(driver);
        self.backend = bend.backend;
        c.glEnable(c.GL_DEPTH_TEST);
        return bend.status;
    }

    pub fn deinitBackend(self: *Render) void {
        self.backend.deinit();
    }

    pub fn setCallback(self: *Render, cb: *const fn (ctx: ?*anyopaque, delta: f64) void, ctx: ?*anyopaque) void {
        self.callback = cb;
        self.context = ctx;
    }

    pub fn pool(self: *Render, eventMode: io.Mode) void {
        const time = &runtime.time;
        time.updateCurrentTime();

        const shouldLimitFPS: bool = runtime.gameUserSettings.getFramerateLimit() > 0 and runtime.gameUserSettings.getVsync() == .disable;
        var frameTime: u64 = 0;
        if (shouldLimitFPS) {
            frameTime = 1000000000 / @as(u64, runtime.gameUserSettings.getFramerateLimit());
        }

        runtime.event.run(eventMode);

        c.glClearColor(0.07, 0.13, 0.17, 1.0);
        c.glClear(c.GL_COLOR_BUFFER_BIT | c.GL_DEPTH_BUFFER_BIT);

        if (self.callback) |cb| cb(self.context, time.delta());

        if (GL_ELEMENT_COUNT > 0)
            c.glDrawElements(c.GL_TRIANGLES, @intCast(runtime.gl.GL_ELEMENT_COUNT), c.GL_UNSIGNED_INT, null);

        if (!c.SDL_GL_SwapWindow(self.backend.window))
            log.sdlErr();

        if (shouldLimitFPS) {
            if (frameTime > 0 and frameTime > time.elapsedNS()) {
                c.SDL_DelayPrecise(frameTime - time.elapsedNS());
            }
        }

        time.updateEndTime();
    }

    pub fn getBackend(self: *Render) *Backend {
        return self.backend;
    }
};

pub const Camera = struct {
    position: za.Vec3,
    orientation: za.Vec3,
    direction: za.Vec3,
    speed: f32,
    sensitivity: f32,

    pub fn init(position: za.Vec3) Camera {
        var self: Camera = undefined;
        self.position = position;
        self.orientation = za.Vec3.new(0.0, 0.0, -1.0);
        self.direction = za.Vec3.zero();
        self.sensitivity = 120.0;
        self.speed = 4.0;
        return self;
    }

    pub fn editorInput(self: *Camera, event: *const c.SDL_Event, w: i32, h: i32) void {
        var input = &runtime.input;
        var inputDir = input.vector(c.SDL_SCANCODE_D, c.SDL_SCANCODE_A, c.SDL_SCANCODE_W, c.SDL_SCANCODE_S);
        self.direction = za.Vec3.norm(za.Vec3.cross(self.orientation, za.Vec3.up())).mul(za.Vec3.set(inputDir.x())).add(za.Vec3.norm(self.orientation).mul(za.Vec3.set(inputDir.y())));
        self.direction = za.Vec3.add(
            za.Vec3.mul(za.Vec3.norm(za.Vec3.cross(self.orientation, za.Vec3.up())), za.Vec3.set(inputDir.x())),
            za.Vec3.mul(za.Vec3.norm(self.orientation), za.Vec3.set(inputDir.y())),
        );

        const yAxis = input.axis(c.SDL_SCANCODE_SPACE, c.SDL_SCANCODE_LCTRL);
        self.direction.yMut().* = yAxis;
        self.speed = if (input.keyPressed(c.SDL_SCANCODE_LSHIFT)) 8.0 else 4.0;

        if (input.mouseButtonPressed(c.SDL_BUTTON_RIGHT)) {
            if (!c.SDL_SetWindowMouseGrab(@ptrCast(runtime.render.backend.window), true)) {
                runtime.log.sdlErr();
            }
            if (!c.SDL_SetWindowRelativeMouseMode(@ptrCast(runtime.render.backend.window), true)) {
                runtime.log.sdlErr();
            }
            if (!c.SDL_HideCursor()) {
                runtime.log.sdlErr();
            }

            if (event.type == c.SDL_EVENT_MOUSE_MOTION) {
                const xrel: f32 = event.motion.xrel;
                const yrel: f32 = event.motion.yrel;

                const rotX: f32 = self.sensitivity * yrel / @as(f32, @floatFromInt(h));
                const rotY: f32 = self.sensitivity * xrel / @as(f32, @floatFromInt(w));

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
            }
        } else {
            if (!c.SDL_SetWindowMouseGrab(@ptrCast(runtime.render.backend.window), false)) {
                runtime.log.sdlErr();
            }
            if (!c.SDL_SetWindowRelativeMouseMode(@ptrCast(runtime.render.backend.window), false)) {
                runtime.log.sdlErr();
            }
            if (!c.SDL_ShowCursor()) {
                runtime.log.sdlErr();
            }
        }
    }

    pub fn editorTick(self: *Camera) void {
        var time = &runtime.time;
        self.direction = za.Vec3.new(std.math.clamp(self.direction.x(), -1.0, 1.0), std.math.clamp(self.direction.y(), -1.0, 1.0), std.math.clamp(self.direction.z(), -1.0, 1.0));
        self.position = self.position.add(self.direction.mul(za.Vec3.set(self.speed)).mul(za.Vec3.set(@floatCast(time.delta()))));
    }

    pub fn matrix(self: *Camera, fovDeg: f32, nearPlane: f32, farPlane: f32, shader: *Shader, uniform: []const u8) void {
        // Initializes matrices since otherwise they will be the null matrix
        var view: za.Mat4 = za.Mat4.set(1.0);
        var projection: za.Mat4 = za.Mat4.set(1.0);

        // Makes camera look in the right direction from the right position
        view = za.lookAt(self.position, self.position.add(self.orientation), za.Vec3.up());

        // Adds perspective to the scene
        var w: i32 = 0;
        var h: i32 = 0;
        if (runtime.render.backend.window) |window| {
            _ = c.SDL_GetWindowSizeInPixels(window, &w, &h);
        }
        projection = za.perspective(fovDeg, @as(f32, @floatFromInt(w)) / @as(f32, @floatFromInt(h)), nearPlane, farPlane);

        const uniformMat = projection.mul(view);
        c.glUniformMatrix4fv(c.glGetUniformLocation(shader.id, uniform.ptr), 1, c.GL_FALSE, &uniformMat.data[0][0]);
    }
};

pub const ElementBuffer = struct {
    id: u32,
    size: i64,

    pub fn init(indices: []const u32, size: i64) ElementBuffer {
        var self: ElementBuffer = undefined;
        self.size = size;
        c.glGenBuffers(1, &self.id);
        c.glBindBuffer(c.GL_ELEMENT_ARRAY_BUFFER, self.id);
        c.glBufferData(c.GL_ELEMENT_ARRAY_BUFFER, size, indices.ptr, c.GL_STATIC_DRAW);
        GL_ELEMENT_COUNT += @divTrunc(size, @sizeOf(u32));
        return self;
    }

    pub fn deinit(self: *ElementBuffer) void {
        GL_ELEMENT_COUNT -= @divTrunc(self.size, @sizeOf(u32));
        if (GL_ELEMENT_COUNT < 0) GL_ELEMENT_COUNT = 0;
        c.glDeleteBuffers(1, &self.id);
    }

    pub fn bind(self: *const ElementBuffer) void {
        c.glBindBuffer(c.GL_ELEMENT_ARRAY_BUFFER, self.id);
    }

    pub fn unbind(self: *const ElementBuffer) void {
        _ = self;
        c.glBindBuffer(c.GL_ELEMENT_ARRAY_BUFFER, 0);
    }
};

pub const Shader = struct {
    id: u32,

    pub fn init(vertexFile: []const u8, fragmentFile: []const u8) struct { status: bool, shader: Shader } {
        const allocator = runtime.defaultAllocator();

        var self: Shader = undefined;
        // Convert the shader source strings into character arrays
        var vertexSource: []u8 = undefined;
        if (io.readFile(vertexFile)) |data| {
            vertexSource = data;
        } else {
            return .{ .status = false, .shader = self };
        }
        defer allocator.free(vertexSource);

        var fragmentSource: []u8 = undefined;
        if (io.readFile(fragmentFile)) |data| {
            fragmentSource = data;
        } else {
            return .{ .status = false, .shader = self };
        }
        defer allocator.free(fragmentSource);

        // Create Vertex Shader Object and get its reference
        const vertexShader = c.glCreateShader(c.GL_VERTEX_SHADER);
        // Attach Vertex Shader source to the Vertex Shader Object
        c.glShaderSource(vertexShader, 1, &vertexSource.ptr, null);
        // Compile the Vertex Shader into machine code
        c.glCompileShader(vertexShader);

        // Create Fragment Shader Object and get its reference
        const fragmentShader = c.glCreateShader(c.GL_FRAGMENT_SHADER);
        // Attach Fragment Shader source to the Fragment Shader Object
        c.glShaderSource(fragmentShader, 1, &fragmentSource.ptr, null);
        // Compile the Vertex Shader into machine code
        c.glCompileShader(fragmentShader);

        // Create Shader Program Object and get its reference
        self.id = c.glCreateProgram();
        // Attach the Vertex and Fragment Shaders to the Shader Program
        c.glAttachShader(self.id, vertexShader);
        c.glAttachShader(self.id, fragmentShader);
        // Wrap-up/Link all the shaders together into the Shader Program
        c.glLinkProgram(self.id);

        // Delete the now useless Vertex and Fragment Shader objects
        c.glDeleteShader(vertexShader);
        c.glDeleteShader(fragmentShader);

        return .{ .status = true, .shader = self };
    }

    pub fn deinit(self: *Shader) void {
        c.glDeleteProgram(self.id);
        self.id = 0;
    }

    pub fn use(self: *const Shader) void {
        c.glUseProgram(self.id);
    }

    pub fn setUniformLocation(self: *const Shader, uniform: []const u8, unit: u32) void {
        // Gets the location of the uniform
        const texuni = c.glGetUniformLocation(self.id, uniform.ptr);
        // Shader needs to be activated before changing the value of a uniform
        self.use();
        // Sets the value of the uniform
        c.glUniform1i(texuni, @intCast(unit));
    }
};

pub const Texture = struct {
    id: u32,
    texType: u32,
    isLoaded: bool,

    pub fn init(textureFile: []const u8, textype: u32, slot: u32, format: u32, pixeltype: u32) struct { status: bool, texture: Texture } {
        var self: Texture = undefined;

        var width: i32 = undefined;
        var height: i32 = undefined;
        var channels: i32 = undefined;

        const data = c.stbi_load(textureFile.ptr, &width, &height, &channels, 0);
        if (data == null) {
            log.err("Failed to load image data.");
            return .{ .status = false, .texture = self };
        }
        defer c.stbi_image_free(data);

        // Generates an OpenGL texture object
        c.glGenTextures(1, &self.id);
        // Assigns the texture to a Texture Unit
        c.glActiveTexture(slot);
        c.glBindTexture(textype, self.id);

        // Configures the type of algorithm that is used to make the image smaller or bigger
        c.glTexParameteri(textype, c.GL_TEXTURE_MIN_FILTER, c.GL_NEAREST_MIPMAP_LINEAR);
        c.glTexParameteri(textype, c.GL_TEXTURE_MAG_FILTER, c.GL_NEAREST);

        // Configures the way the texture repeats (if it does at all)
        c.glTexParameteri(textype, c.GL_TEXTURE_WRAP_S, c.GL_REPEAT);
        c.glTexParameteri(textype, c.GL_TEXTURE_WRAP_T, c.GL_REPEAT);

        // Extra lines in case you choose to use GL_CLAMP_TO_BORDER
        // float flatColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, flatColor);

        // Assigns the image to the OpenGL Texture object
        c.glTexImage2D(textype, 0, c.GL_RGBA, width, height, 0, format, pixeltype, data);
        // Generates MipMaps
        c.glGenerateMipmap(textype);

        // Deletes the image data as it is already in the OpenGL Texture object
        //stbi_image_free(bytes);

        // Unbinds the OpenGL Texture object so that it can't accidentally be modified
        c.glBindTexture(textype, 0);

        self.isLoaded = true;

        self.texType = textype;
        return .{ .status = true, .texture = self };
    }

    pub fn deinit(self: *Texture) void {
        c.glDeleteTextures(1, &self.id);
        self.id = 0;
        self.isLoaded = false;
        self.texType = 0;
    }

    pub fn bind(self: *const Texture) void {
        c.glBindTexture(self.texType, self.id);
    }

    pub fn unbind(self: *const Texture) void {
        c.glBindTexture(self.texType, 0);
    }
};

pub const VertexBuffer = struct {
    id: u32,

    pub fn init(vertices: []const f32, size: i64) VertexBuffer {
        var self: VertexBuffer = undefined;
        c.glGenBuffers(1, &self.id);
        c.glBindBuffer(c.GL_ARRAY_BUFFER, self.id);
        c.glBufferData(c.GL_ARRAY_BUFFER, size, vertices.ptr, c.GL_STATIC_DRAW);
        return self;
    }

    pub fn deinit(self: *VertexBuffer) void {
        c.glDeleteBuffers(1, &self.id);
        self.id = 0;
    }

    pub fn bind(self: *const VertexBuffer) void {
        c.glBindBuffer(c.GL_ARRAY_BUFFER, self.id);
    }

    pub fn unbind(self: *const VertexBuffer) void {
        _ = self;
        c.glBindBuffer(c.GL_ARRAY_BUFFER, 0);
    }
};

pub const VertexArray = struct {
    id: u32,

    pub fn init() VertexArray {
        var self: VertexArray = undefined;
        c.glGenVertexArrays(1, &self.id);

        return self;
    }

    pub fn deinit(self: *VertexArray) void {
        c.glDeleteVertexArrays(1, &self.id);
        self.id = 0;
    }

    pub fn bind(self: *const VertexArray) void {
        c.glBindVertexArray(self.id);
    }

    pub fn unbind(self: *const VertexArray) void {
        _ = self;
        c.glBindVertexArray(0);
    }

    pub fn enableAttrib(self: *const VertexArray, vertexBuf: *VertexBuffer, layout: u32, num: u32, attribType: u32, stride: i64, offset: ?*const anyopaque) void {
        _ = self;
        vertexBuf.bind();
        c.glVertexAttribPointer(layout, @intCast(num), attribType, c.GL_FALSE, @intCast(stride), offset);
        c.glEnableVertexAttribArray(layout);
        vertexBuf.unbind();
    }
};
