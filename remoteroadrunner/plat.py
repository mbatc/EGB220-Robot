from sdl2 import *

import ctypes

class Window:
  def __init__(self, width, height, name):
    if SDL_Init(SDL_INIT_EVERYTHING) < 0:
      print("Error: SDL could not initialize! SDL Error: " + SDL_GetError().decode("utf-8"))
      exit(1)

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)

    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, b"1")
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, b"1")

    self.sdl_window = SDL_CreateWindow(
      name.encode('utf-8'),
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height,
      SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
    )

    self.gl_context = SDL_GL_CreateContext(self.sdl_window)
    if self.gl_context is None:
      print("Error: Cannot create OpenGL Context! SDL Error: " + SDL_GetError().decode("utf-8"))
      exit(1)

    SDL_GL_MakeCurrent(self.sdl_window, self.gl_context)
    if SDL_GL_SetSwapInterval(1) < 0:
      print("Warning: Unable to set VSync! SDL Error: " + SDL_GetError().decode("utf-8"))
      exit(1)

  def title(self):
    pass

  def width(self):
    w = c_int()
    SDL_GetWindowSize(self.sdl_window, ctypes.byref(w), None)
    return w.value

  def height(self):
    h = c_int()
    SDL_GetWindowSize(self.sdl_window, None, ctypes.byref(h))
    return h.value

  def x(self):
    x = c_int()
    SDL_GetWindowPosition(self.sdl_window, ctypes.byref(x), None)
    return x.value

  def y(self):
    y = c_int()
    SDL_GetWindowPosition(self.sdl_window, None, ctypes.byref(y))
    return y.value
