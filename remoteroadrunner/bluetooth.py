from bleak import BleakClient
from threading import Lock
import queue
import asyncio
import time
import imgui

MODEL_NBR_UUID = "00002a24-0000-1000-8000-00805f9b34fb"

read_characteristic_id = "0000ffe1"

def full_characteristic_id(id, suffix = "-0000-1000-8000-00805f9b34fb"):
  return id + suffix

class Connection:
  def __init__(self, app, address):
    self.address         = address # MAC address of the BT module
    self.client          = BleakClient(address)
    # Serial characteristic of the bluetooth module
    self.read_char       = full_characteristic_id(read_characteristic_id)
    self.messages_out    = queue.Queue() # Outgoing messages
    self.messages_in     = queue.Queue() # Outgoing messages
    self.handler         = None
    self._connect_failed = False         # Flag to indicate if the connection was successfuly
    self.message_lock    = Lock()
    self.accum_buffer    = bytearray()   # Temp buffer to accumulate incoming packets in
    self.connected       = False         # Is the BT connection active
    self.running         = True          # Is the worker task running
    self.timeout         = 5.0           # Response timeout
    self.current_msg     = None          # The current message awaiting a response
    self.app             = app
    # Try to connect to the bluetooth device
    self.connect_task    = asyncio.create_task(self.__connect())

    # Start the worker task
    self.worker          = asyncio.create_task(self.worker_task())

  def get_connect_task(self):
    return self.connect_task

  def connect_failed(self):
    return self._connect_failed

  def handle_messages(self):
    while True:
      try:
        message = self.messages_in.get(False, None)
      except:
        break

      self.handler(message)


  def set_response_handler(self, handler):
    self.handler = handler

  def enqueue_message(self, message):
    '''
    Add a message to be sent to the bluetooth module

    A message should have a response handler set which will get
    called when a response is available.
    '''
    self.messages_out.put(message)

  def __notify(self, sender: int, data: bytearray):
    '''
    Recieves incoming data from the bluetooth connection
    and assembles packets.

    The packet is either given to the current message as a response,
    or added to the incoming packet queue.
    '''
    try:
      for b in data:
        if b == 0:
          # Construct a packet from the data
          recieved = self.accum_buffer.decode('utf-8')
  
          self.app.log(['BT Recv:', recieved], [imgui.Vec4(0.3, 0.8, 0.3, 1), None])

          # If a message is waiting for a response, set it
          if self.current_msg != None:
            self.current_msg.set_response(recieved)
            self.current_msg = None
          else: # Otherwise add the packet to the incoming queue
            self.messages_in.put(recieved)
          # Clear the byte buffer
          self.accum_buffer = bytearray()
        else:
          self.accum_buffer.append(b) # Add byte to the buffer until we have a full packet
    except Exception as e:
      print("Notfy failed: " + str(e))

  async def worker_task(self):
    '''
    Sends messages placed in the message queue.
    Only sends 1 message at a time and always expects a response for a message.
    Response will timeout after 1 ssecond
    '''
    while (self.running):
      try:
        next_message = None
        try:
          next_message = self.messages_out.get(False, None)
        except Exception as e:
          await asyncio.sleep(0.001) # Sleep for 1ms
          continue
  
        # If there was a message, send it and wait for the response
        try:
          # Flush previous data
          # Helps stops fail messages from cascading
          await self.client.write_gatt_char(self.read_char, [ 0 ])
            
          # Set the current message before sending the command
          self.current_msg = next_message

          # Send packet bytes
          for c in next_message.packet.encode('utf-8'):
            await self.client.write_gatt_char(self.read_char, [ c ])
          await self.client.write_gatt_char(self.read_char, [ 0 ])
        except Exception as e:
          print("Failed to send command: " + str(e))
          continue
  
        # Record the time the packet was sent so we can test for a timeout
        send_time = time.time()
  
        # Wait for the messages response packet to be set
        while not self.current_msg.has_response():
          await asyncio.sleep(0.001) # Sleep for 1ms      
  
          # Check if the response has timed out
          if time.time() - send_time > self.timeout:
            self.current_msg.timed_out = True # Signal the timeout was reached
            break
  
        self.current_msg = None
      except Exception as e:
        print("BT Worker Exception: " + str(e))


  async def __connect(self):
    '''
    Connects to the bluetooth module and sets up the 
    notify function which listens for incoming data.
    '''
    await self.client.connect()
    # self._connect_failed = not await self.client.is_connected()
    await self.client.start_notify(self.read_char, self.__notify)
    self._connect_failed = not self.client.is_connected
    self.connected = True
