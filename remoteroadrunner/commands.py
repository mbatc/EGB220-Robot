import queue
import asyncio
import bluetooth
import imgui

from message import Message
from packet import Packet, PacketType

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
    self.bt.set_response_handler(self.__bt_response_handler)
    # Return the connect task
    return self.bt.get_connect_task()

  def __bt_packet_handler(self, packet : Packet):
    if (packet.type == PacketType.TRACK_SEC):
      pass

  def get_lap_times(self):
    pass

  def get_track_details(self):
    pass

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
    packet = Packet()
    packet.make_call(name)
    self.send(
      Message(packet)
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
      packet = Packet()
      packet.make_set_var(name, self.variables[name])
      self.send(
        Message()
          .on_response(lambda sent, response: None)
      )
    else:
      packet = Packet()
      packet.make_get_var(name)
      self.send(
        Message()
          .on_response(self.handle_get)
      )

  def sync_command_list(self):
    '''
    Fetch the command list from the device
    '''
    packet = Packet()
    packet.make_list_commands()
    self.send(
      Message(packet)
        .on_response(self.handle_command_list)
    )

  def sync_variable_list(self):
    '''
    Fetch the variable list from the device
    '''
    packet = Packet()
    packet.make_list_var()
    self.send(
      Message(packet)
        .on_response(self.handle_variable_list)
    )

  def handle_command_list(self, sent : Packet, response : Packet):
    if response.type != PacketType.OK_LIST_CMD:
      return

    try:
      self.commands = response.parse_ok_list_commands()
    except Exception as e:
      print("Failed to parse command list: {0}".format(e))


  def handle_variable_list(self, sent : Packet, response : Packet):
    if response.type != PacketType.OK_LIST_VAR:
      return

    added = []
    existing = []

    # Added missing variables
    for var_def in response.parse_ok_list_var():
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

      
  def handle_get(self, sent : Packet, response : Packet):
    try:
      value = response.parse_ok_get()
      var_name = sent.parse_get()
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
