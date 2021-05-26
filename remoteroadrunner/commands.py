import serial_interface
import queue
import asyncio
import bluetooth
import imgui

from message import Message
class RoadRunnerContext:
  def __init__(self, app):
    self.commands = [  ]
    self.variables = {  }
    self.bt = None
    self.get_queue = queue.Queue()
    self.app = app
    self.track_details = []
    self.lap_times     = []

  # self.app.log([ "BT Recv: ", msg.strip()], [imgui.Vec4(0.3, 0.8, 0.3, 1), None])

  def connect(self, address):
    # Create the connection
    self.bt = bluetooth.Connection(address)
    self.bt.set_response_handler(self.__bt_message_handler)
    # Return the connect task
    return self.bt.get_connect_task()

  def __bt_message_handler(self, recieved):
    self.app.log(['Unhandled BT Message:', recieved])
    pass

  def get_lap_times(self):
    return self.lap_times

  def get_track_details(self):
    return self.track_details

  def is_connected(self):
    return self.bt != None and self.bt.connected

  def get_commands(self):
    return self.commands

  def get_variables(self):
    return self.variables.keys()

  def get_var_type(self, name):
    return type(self.get_var(name))

  def get_var(self, name):
    '''
    Get the local value of a variable
    '''
    return self.variables[name]

  def set_var(self, name, value):
    '''
    Set the local value of a variable
    '''
    if (type(self.variables[name]) == type(value)):
      sync = self.variables[name] != value
      self.variables[name] = value
      if sync:
        self.sync_var(name, True)

  def call_command(self, name):
    '''
    Call a command on the device
    '''
    self.send(
      Message(serial_interface.call_command(name))
        .on_response(lambda packet, response : None)
    )

  def sync_var(self, name, apply=False):
    '''
    Sync the variable state with the arduino value.
    If apply is True, the value in this app will be sent to the device.
    If apply is False, the value will be fetched from the arduino.
    '''
    if apply:
      if name not in self.variables:
        return
      self.send(
        Message(serial_interface.set_var(name, self.variables[name]))
          .on_response(lambda sent, response: None)
      )
    else:
      self.send(
        Message(serial_interface.get_var(name))
          .on_response(self.handle_get)
      )

  def sync_command_list(self):
    '''
    Fetch the command list from the device
    '''
    self.send(
      Message(serial_interface.list_commands())
        .on_response(self.handle_command_list)
    )

  def sync_variable_list(self):
    '''
    Fetch the variable list from the device
    '''
    self.send(
      Message(serial_interface.list_vars())
        .on_response(self.handle_variable_list)
    )

  def handle_command_list(self, sent, response):
    self.commands = serial_interface.parse_response_lscmd(response)


  def handle_variable_list(self, sent, response):
    if not serial_interface.response_is_lsvar(response):
      return

    added = []
    existing = []

    # Added missing variables
    for var_def in serial_interface.parse_response_lsvar(response):
      name = var_def["name"]
      var_type = var_def["type"]

      if name in self.variables:
        var = self.variables[name]
        if (isinstance(var, var_type)):
          var = var_type()
          existing.append(name)
        else:
          added.append(name)
      else:
        added.append(name)

    # Create a new dictionary, only containing the variables
    # on the remote device
    prev_vars = self.variables
    self.variables = {}
    for name in existing:
      self.variables[name] = prev_vars[name]

    # Fetch the value of variables that were added
    for var in added:
      self.sync_var(var)

      
  def handle_get(self, sent, response):
    try:
      var_name, value = serial_interface.parse_response_get(message)
      self.variables[var_name] = value
    except Exception as e:
      print("Failed to get variable: {0}".format(e))

  def send(self, message):
    if self.bt == None:
      self.app.log([ "Failed to Send:", "Not Connected", "{" + str(message.packet) + "}" ], [imgui.Vec4(0.8, 0.3, 0.3, 1), None, imgui.Vec4(0.3, 0.8, 0.3, 1)])
      return False

    self.app.log([ "BT Send: ", str(message.packet)], [imgui.Vec4(0.3, 0.3, 0.8, 1), None])
    self.bt.enqueue_message(message)
    return True
