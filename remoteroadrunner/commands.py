import serial_interface
import queue
import asyncio
import bluetooth
import imgui

class CommandContext:
  def __init__(self, app):
    self.commands = [ 'command1', 'command2' ]
    self.variables = { 'test': int(1), 'test2': float(3.2), 'test3': bool(False) }
    self.bt = None
    self.get_queue = queue.Queue()
    self.app = app

  def __bt_response_handler(self, msg):
    if serial_interface.response_is_lscmd(msg):
      self.handle_command_list(msg)
    elif serial_interface.response_is_lsvar(msg):
      self.handle_variable_list(msg)
    elif serial_interface.response_is_type(msg):
      self.handle_type(msg)
    elif serial_interface.response_is_get(msg):
      self.handle_get(msg)
    self.app.log([ "BT Recv: ", msg.strip()], [imgui.Vec4(0.3, 0.8, 0.3, 1), None])

  def connect(self, address):
    # Create the connection
    self.bt = bluetooth.Connection(address)
    self.bt.set_response_handler(self.__bt_response_handler)
    # Return the connect task
    return self.bt.get_connect_task()

  def is_connected(self):
    return self.bt != None

  def call_command(self, name):
    '''
    Call a command on the device
    '''
    self.send(serial_interface.call_command(name))

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

  def sync_var(self, name, apply=False):
    '''
    Sync the variable state with the arduino value.
    If apply is True, the value in this app will be sent to the device.
    If apply is False, the value will be fetched from the arduino.
    '''
    if apply:
      if name not in self.variables:
        return
      message = serial_interface.set_var(name, self.variables[name])
      self.send(message)
    else:
      message = serial_interface.get_var(name)
      self.send(message)

  def sync_command_list(self):
    '''
    Fetch the command list from the device
    '''
    self.send(serial_interface.list_commands())

  def sync_variable_list(self):
    '''
    Fetch the variable list from the device
    '''
    self.send(serial_interface.list_vars())

  def handle_command_list(self, message):
    self.commands = serial_interface.parse_response_lscmd(message)

  def handle_variable_list(self, message):
    added = []
    existing = []

    # Added missing variables
    for var_def in serial_interface.parse_response_lsvar(message):
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

      
  def handle_get(self, message):
    try:
      var_name, value = serial_interface.parse_response_get(message)
      self.variables[var_name] = value
    except Exception as e:
      print("Failed to get variable: {0}".format(e))

  def handle_type(self, message):
    value = serial_interface.parse_response_type(message)

  def send(self, message):
    if self.bt == None:
      return False

    self.app.log([ "BT Send: ", message.strip()], [imgui.Vec4(0.3, 0.3, 0.8, 1), None])
    self.app.task_queue.enqueue(self.bt.send(message))
    return True
