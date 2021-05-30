
# 3rd Party Libs
import asyncio
import time
import os
import OpenGL.GL as gl
from imgui.integrations.sdl2 import SDL2Renderer

import glfw
import OpenGL.GL as gl

import imgui
from imgui.integrations.glfw import GlfwRenderer

from sdl2 import *
import ctypes
from bleak import BleakScanner
from datetime import datetime

# Local modules
import bluetooth
import plat
import gui
from commands import RoadRunnerContext

class Device:
  def __init__(self, name, address, manufacturer):
    self.name = name.strip()
    self.address = address
    self.manufacturer = manufacturer

    if len(self.name) == 0:
      self.name = "Unknown"

  def detailed(self):
    return "{0} [{1}][addr: {2}]".format(self.name, self.manufacturer, self.address)

class Command:
  def __init__(self, func, arg_types):
    self.func = func
    self.arg_types = arg_types

  def __call__(self, *args):
    converted = [ self.arg_types[i](args[i]) for i in range(len(args)) ]
    return self.func(*converted)

class AppCommands:
  def __init__(self, app):
    self.app = app
    self.commands = {}
    self.add("list_commands", self.list_commands)

  def list_commands(self):
    return '\n'.join([ name for name in self.commands.keys() ])

  def add(self, name, func, arg_types = []):
    self.commands[name] = Command(func, arg_types)

  def call(self, command):
    args = command.split()
    if len(args) == 0:
      return "Cannot call empty command"

    cmd_name = args[0]
    cmd_args = args[1:]
    if cmd_name in self.commands:
      try:
        return self.commands[args[0]](*args[1:])
      except Exception as e:
        return "Exception Raised: {0}".format(str(e))
    return "Command '{0}' does not exist".format(cmd_name)

class App:
  def __init__(self):
    imgui.create_context()

    self.context  = RoadRunnerContext(self)
    self.running  = True
    self.window   = plat.Window(1280, 720, "Remote Road Runner")
    self.renderer = SDL2Renderer(self.window.sdl_window)
    self.gui      = gui.GUI(self)
    self.scanner  = BleakScanner()
    self.scanner.register_detection_callback(self.__on_device_found)
    self.connecting = False

    self.is_scanning = False
    self.console_in  = []
    self.console_log = []
    self.log_time    = True

    self.commands = AppCommands(self)
    self.devices  = { 
      # "64:69:4E:7B:5E:0B": Device("roadrunner", "64:69:4E:7B:5E:0B", "chief egb220 engineers")
    }

    self.connected_device = ""
    self.register_console_commands()

  def __del__(self):
    '''
    Cleanup the application data on destroy
    '''
    if self.renderer   != None: self.renderer.shutdown()
    SDL_Quit()

  def set_log_time(self, enabled):
    self.log_time = bool(enabled)

  def start_scanner(self):
    asyncio.create_task(self.scanner.start())
    self.is_scanning = True
    self.log("Started scanning...")

  def stop_scanner(self):
    asyncio.create_task(self.scanner.stop())
    self.is_scanning = False
    self.log("Stopped scanning...")

  def log(self, message, color=None):
    entry = []
    if self.log_time:
      date_text = "[{0}]".format(str(datetime.now().time()))
      entry.append({"col": imgui.get_style_color_vec_4(imgui.COLOR_PLOT_HISTOGRAM), "text": date_text})

    if isinstance(message, list) and isinstance(color, list):
      for msg, col in zip(message, color):
        entry.append({ "col": col, "text": msg })
    else:
      entry.append({ "col": color, "text": message })

    self.console_log.append(entry)

  async def _connect_async(self, address):
    self.log("Connecting to {0}".format(address))
    await self.context.connect(address)
    if not self.context.bt.connect_failed():
      self.connected_device = address
      self.log("Connected to {0}".format(address))
    else:
      self.log("Failed to connect to {0}".format(address))

  def connect(self, address):
    self.connecting = True
    asyncio.create_task(self._connect_async(address))

  def call_command(self, name):
    self.context.call_command(name)
  
  def refresh_commands(self):
    self.context.sync_command_list()

  def refresh_variables(self):
    self.context.sync_variable_list()

  def __on_device_found(self, device, adv_data):
    man_name = "Unknown"
    if len(device.name) == 0:
      return # Ignore unnamed devices

    new_device = Device(device.name, device.address, man_name)
    if device.address not in self.devices:
      self.log(["Found BT Device: ", new_device.detailed()], [imgui.Vec4(0.3, 0.8, 0.3, 1), None])
    self.devices[device.address] = new_device

  def update(self):
    self.gui.update()

    if self.connecting and self.context.is_connected():
      self.refresh_commands()
      self.refresh_variables()
      self.connecting = False

    self.context.handle_incoming()

    for cmd in self.console_in:
      self.process_console(cmd)
    self.console_in = []

  def clear_console(self):
    self.console_log.clear()

  def process_console(self, cmd):
    self.log(["> ", cmd], [imgui.get_style_color_vec_4(imgui.COLOR_SEPARATOR_ACTIVE), None])
    result = self.commands.call(cmd)
    if result != None:
      try:
        self.log(str(result))
      except:
        self.log("Success, but cannot convert the returned value to a string")

  def process_events(self):
    event = SDL_Event()
    while SDL_PollEvent(ctypes.byref(event)) != 0:
      if event.type == SDL_QUIT:
        self.running = False
        break
      self.renderer.process_event(event)
    self.renderer.process_inputs()

  async def run(self):
    while self.running:
      self.process_events()
      self.update()
      self.render()
      await asyncio.sleep(0.001)

  def render(self):
    imgui.new_frame()
    gl.glClearColor(1., 1., 1., 1)
    gl.glClear(gl.GL_COLOR_BUFFER_BIT)
    self.gui.draw()
    imgui.render()
    self.renderer.render(imgui.get_draw_data())
    SDL_GL_SwapWindow(self.window.sdl_window)

  def register_console_commands(self):
    self.commands.add("clear", self.clear_console)
    self.commands.add("call", self.call_command, [ str ])
    self.commands.add("refresh_variables", self.refresh_variables)
    self.commands.add("refresh_commands", self.refresh_commands)
    self.commands.add("connect", self.connect, [ str ])
    self.commands.add("log_time", self.set_log_time, [ bool ])
    self.commands.add("start_scanner", self.start_scanner)
    self.commands.add("stop_scanner", self.stop_scanner)


# Main program loop
async def main():
  app = App()
  await app.run()

if __name__=="__main__":
  asyncio.run(main())
