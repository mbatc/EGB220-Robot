from bleak import BleakClient
from threading import Lock
import queue
import asyncio

MODEL_NBR_UUID = "00002a24-0000-1000-8000-00805f9b34fb"

read_characteristic_id = "0000ffe1"

def full_characteristic_id(id, suffix = "-0000-1000-8000-00805f9b34fb"):
  return id + suffix

class Connection:
  def __init__(self, address):
    # Setup members
    self.address         = address
    self.client          = BleakClient(address)
    self.read_char       = full_characteristic_id(read_characteristic_id)
    self.callback_queue  = queue.Queue()
    self._connect_failed = False
    self.handler         = None
    self.send_lock       = Lock()
    self.accum_buffer    = bytearray()

    # Try to connect to the bluetooth device
    self.connect_task    = asyncio.create_task(self.__connect())

  def get_connect_task(self):
    return self.connect_task

  def connect_failed(self):
    return self._connect_failed

  def set_response_handler(self, handler):
    self.handler = handler

  async def send(self, message):
    # Add a task to send the message
    try:
      for c in message.encode('utf-8'):
        await self.client.write_gatt_char(self.read_char, [c])
    except Exception as e:
      print("Failed to send command: " + str(e))

  def __notify(self, sender: int, data: bytearray):
    for b in data:
      if b == 0:
        msg = self.accum_buffer.decode('utf-8')
        if self.handler != None:
          self.handler(msg)
          print(msg)
        else:
          print(msg)
        self.accum_buffer = bytearray()
      else:
        self.accum_buffer.append(b)

  async def __connect(self):
    await self.client.connect()
    # self._connect_failed = not await self.client.is_connected()
    await self.client.start_notify(self.read_char, self.__notify)
    self._connect_failed = not self.client.is_connected


