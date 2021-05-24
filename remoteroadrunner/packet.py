from enum import Enum

class PacketType(Enum):
  ERROR    = 0x01

  CALL     = 0x02
  LIST_CMD = 0x03
  LIST_VAR = 0x04
  SET_VAR  = 0x05
  GET_VAR  = 0x06
  
  OK_GET      = 0x07
  OK_SET      = 0x08
  OK_CALL     = 0x09
  OK_TYPE     = 0x0A
  OK_LIST_CMD = 0x0B
  OK_LIST_VAR = 0x0C
  
  TRACK_START = 0x0D
  TRACK_FIN   = 0x0E
  TRACK_SEC   = 0x0F
  
  UNKNOWN     = 0x0D

class PacketSpec(Enum):
  FLUSH_PACKET = 0x0

class VarType(Enum):
  FLOAT = 0x01
  INT   = 0x02
  BOOL  = 0x03

class TrackType(Enum):
  STRAIGHT = 0x01
  RTURN    = 0x02
  LTURN    = 0x03

def get_var_type(type_enum):
  if type_enum == VarType.FLOAT:
    return float
  elif type_enum == VarType.INT:
    return int
  elif type_enum == VarType.BOOL:
    return bool
  return None

class Packet:
  def __init__(self):
    self.type = PacketType.UNKNOWN
    self.data = []

  # WRITING OUTGOING PACKETS ARE IMPLEMENTED HERE
  #
  # READING DATA IS NOT IMPLEMENTED IN PYTHON AS IT
  # IS NOT REQUIRED ON THIS END OF THE COMMUNICATION

  def make_packet(self, type, data):
    self.type = type
    self.data = data

  def make_call(self, command_name):
    '''
    Write data for a 'call' packet
    '''
    self.make_packet(PacketType.CALL, command_name.encode('utf-8'))

  def make_set_var(self, name, value):
    '''
    Write data for a 'set_var' packet
    '''
    value_text = str(value)

    if isinstance(value, float):
      # Remove trailing 0's
      value_text = value_text.strip('0')
      # Remove . if all characters to right are 0
      value_text = value_text.strip('.')
      # If string is empty, float was 0
      if len(value_text) == 0:
        value_text = '0'
    self.make_packet(PacketType.SET_VAR, (name + ' ' + value_text).encode('utf-8'))
  
  def make_get_var(self, name):
    '''
    Write data for a 'get_var' packet
    '''
    self.make_packet(PacketType.LIST_CMD, name.encode('utf-8'))

  def parse_get_var(self):
    return self.data.decode('utf-8')

  def make_list_var(self):
    '''
    Write data for a 'list_var' packet
    '''
    self.make_packet(PacketType.LIST_VAR, [])

  def make_list_commands(self):
    '''
    Write data for a 'list_cmd' packet
    '''
    self.make_packet(PacketType.LIST_CMD, [])

  def to_serial(self):
    '''
    Convert the packet to a serial message
    '''
    return [ self.type ] + self.data + [ PacketSpec.FLUSH_PACKET ]

  # PARSING INCOMING PACKETS IS IMPLEMENTED HERE
  #
  # WE DON'T IMPLEMENT WRITING THEM AS IT IS NOT
  # REQUIRED ON THE PYTHON SIDE OF THE COMMUNICATION
  def parse_ok_list_var(self):
    message = self.data.decode('utf-8')
    lines = message.split('\n')
    count = int(lines[0])
    return lines[1:1+count]

  def parse_ok_list_commands(self):
    message = self.data.decode('utf-8')
    lines = message.split('\n')
    count = int(lines[0])
    var_list = []
    for var in lines[1:1+count]:
      var_list.append({ 'name': var[1:], 'type': get_var_type(var[0])})
    return var_list

  def parse_ok_get(self):
    desc = self.data.decode('utf-8')
    var_type = get_var_type(desc[0])
    value = desc[1:]
    if var_type is bool:
      return True if value != '0' else False
    return var_type(value)

  def parse_track_section(self):
    motor_diff = self.data[0] - 128
    length     = self.data[1]
    return motor_diff, length

  def from_serial(self, message):
    '''
    Convert a serial message to a packet
    '''
    self.type = PacketType.UNKNOWN
    self.data = []
    if len(message) > 0:
      self.type = PacketType(message[0])
      self.data = message[1:]

  def __str__(self):
    return str(self.to_serial())

