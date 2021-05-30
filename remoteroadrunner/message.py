
class Message:
  def __init__(self, packet):
    '''
    Create a new message.
    '''
    self.packet   = packet
    self.response = None
    self.response_handler = None
    self.timed_out = False

  def set_response(self, response):
    '''
    Set the response data.
    
    This will call the response handler if it exists
    '''
    # Set the response
    self.response = response

    # Call the response handler with the packet and response
    if self.response_handler != None:
      self.response_handler(self.packet, self.response)

  def on_response(self, handler):
    '''
    Set the response handler
    '''
    self.response_handler = handler
    
    return self
    
  def has_response(self):
    '''
    Check if the message has recieved a response
    '''
    return self.response != None

  def timeout_reached(self):
    return self.timed_out