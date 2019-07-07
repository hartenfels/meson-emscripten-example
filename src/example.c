/*
 * Copyright (c) 2019 Carsten Hartenfels
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <cglm/cglm.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define STOP_RUNNING 0
#define KEEP_RUNNING 1


static const GLfloat cube_vertices[] = {
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
};

static const GLfloat cube_uvs[] = {
    0.00f, 1.00f,
    0.00f, 0.66f,
    0.33f, 0.66f,
    1.00f, 1.00f,
    0.66f, 0.66f,
    0.99f, 0.66f,
    0.66f, 0.66f,
    0.33f, 0.33f,
    0.66f, 0.33f,
    1.00f, 1.00f,
    0.66f, 1.00f,
    0.66f, 0.66f,
    0.00f, 1.00f,
    0.33f, 0.66f,
    0.33f, 1.00f,
    0.66f, 0.66f,
    0.33f, 0.66f,
    0.33f, 0.33f,
    1.00f, 0.33f,
    1.00f, 0.66f,
    0.66f, 0.66f,
    0.66f, 1.00f,
    0.33f, 0.66f,
    0.66f, 0.66f,
    0.33f, 0.66f,
    0.66f, 1.00f,
    0.33f, 1.00f,
    0.00f, 0.66f,
    0.00f, 0.33f,
    0.33f, 0.33f,
    0.00f, 0.66f,
    0.33f, 0.33f,
    0.33f, 0.66f,
    0.66f, 0.33f,
    1.00f, 0.33f,
    0.66f, 0.66f,
};

static SDL_Window    *window;
static SDL_GLContext *glcontext;

static GLuint program;
static GLuint attributes[2];
static GLuint texture;

static struct {
    GLint model;
    GLint view;
    GLint projection;
    GLint sampler;
} uniform;

static struct {
    mat4 model;
    mat4 view;
    mat4 projection;
} matrix;


static void eg_exit(void)
{
    if (glcontext) {
        SDL_GL_DeleteContext(glcontext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    IMG_Quit();
    SDL_Quit();
}

static noreturn void die(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    eg_exit();
    abort();
}


char *slurp(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        die("Can't open '%s': %s", path, strerror(errno));
    }

    fseek(fp, 0L, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char *buf = malloc(len + 1);
    memset(buf, 0, len + 1);
    fread(buf, 1, len, fp);

    fclose(fp);
    return buf;
}

static GLuint compile_shader(GLenum type, const char *path)
{
    GLuint shader = glCreateShader(type);
    char   *code  = slurp(path);

    const GLchar *source = code;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    free(code);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        die("Can't compile shader: %s", path);
    }

    return shader;
}

static void init_shader(void)
{
    program = glCreateProgram();

    GLuint vertex_shader   = compile_shader(GL_VERTEX_SHADER,   "assets/example.vert");
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, "assets/example.frag");

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        die("Can't link shaders");
    }

    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);

    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);
}


static void init_args(void)
{
    glGenBuffers(2, attributes);
    uniform.model      = glGetUniformLocation(program, "model");
    uniform.view       = glGetUniformLocation(program, "view");
    uniform.projection = glGetUniformLocation(program, "projection");
    uniform.sampler    = glGetUniformLocation(program, "sampler");
}


static void resize(void)
{
    int width;
    int height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glm_perspective(60.0f * (CGLM_PI / 180.0f), ((float) width) / ((float) height),
                    0.1f, 1000.0f, matrix.projection);
}

static void init_matrices(void)
{
    vec3 eye = {4.0f, 3.0f, -3.0f};
    vec3 dir = {0.0f, 0.0f,  0.0f};
    vec3 up  = {0.0f, 1.0f,  0.0f};
    glm_lookat(eye, dir, up, matrix.view);
    glm_mat4_identity(matrix.model);
    resize();
}


static void init_texture(void)
{
    SDL_Surface *surface = IMG_Load("assets/texture.png");
    if (!surface) {
        die("IMG_Load: %s", IMG_GetError());
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h,
                 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(surface);
}


static void eg_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        die("SDL_Init: %s", SDL_GetError());
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        die("IMG_Init: %s", IMG_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("example", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        die("SDL_CreateWindow: %s", SDL_GetError());
    }

    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        die("SDL_GL_CreateContext: %s", SDL_GetError());
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        die("glewInit: %s", "failed");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    init_shader();
    init_args();
    init_matrices();
    init_texture();
}


static int pump_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return STOP_RUNNING;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    return STOP_RUNNING;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    resize();
                }
                break;
            default:
                break;
        }
    }
    return KEEP_RUNNING;
}


static void think(void)
{
    glm_rotate_x(matrix.model, 0.012f, matrix.model);
    glm_rotate_y(matrix.model, 0.033f, matrix.model);
}

static void render(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    glUniformMatrix4fv(uniform.model,      1, GL_FALSE, *matrix.model);
    glUniformMatrix4fv(uniform.view,       1, GL_FALSE, *matrix.view);
    glUniformMatrix4fv(uniform.projection, 1, GL_FALSE, *matrix.projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(uniform.sampler, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, attributes[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices),
                 cube_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, attributes[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs),
                 cube_uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glDrawArrays(GL_TRIANGLES, 0, (sizeof(cube_vertices) / sizeof(cube_vertices[0])) / 3);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);

    SDL_GL_SwapWindow(window);
}

static void step(void)
{
    think();
    render();
}


#ifdef __EMSCRIPTEN__
static void run_emscripten(void)
{
    if (pump_events() == KEEP_RUNNING) {
        step();
    }
    else {
        eg_exit();
        emscripten_cancel_main_loop();
    }
}
#endif

void eg_run(void)
{
    eg_init();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(run_emscripten, 0, false);
#else
    while (pump_events() == KEEP_RUNNING) {
        step();
    }
    eg_exit();
#endif
}
