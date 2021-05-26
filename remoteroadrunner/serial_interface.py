
def call_command(name):
  return "call {0}\n".format(name)

def set_var(name, value):
  if isinstance(value, float): # Special case for floats
    return "set {0} {1:.4f}\n".format(name, value)
  else:
    return "set {0} {1}\n".format(name, value)

def get_var(name):
  return "get {0}\n".format(name)

def get_type(name):
  return "type {0}\n".format(name)

def list_commands():
  return "lscmd\n"

def list_vars():
  return "lsvar\n"

def get_var_type(name):
  if name == 'f32':
    return float
  elif name == 'f64':
    return float
  elif name == 'i32':
    return int
  elif name == 'b':
    return bool
  return None

def response_is_error(response):
  return response.startswith('ERR+') 

def response_is_get(response):
  return response.startswith('OK+GET')

def response_is_set(response):
  return response.startswith('OK+SET')

def response_is_call(response):
  return response.startswith('OK+CALL')

def response_is_type(response):
  return response.startswith('OK+TYPE')

def response_is_lscmd(response):
  return response.startswith('OK+LSCMD')

def response_is_lsvar(response):
  return response.startswith('OK+LSVAR')

def parse_response_lscmd(response):
  if (not response_is_lscmd(response)):
    return []
    
  lines = response.split('\n')[1:]
  count = int(lines[0])
  return lines[1:1+count]

def parse_response_lsvar(response):
  if (not response_is_lsvar(response)):
    return []

  lines = response.split('\n')[1:]
  count = int(lines[0])
  var_list = []
  for var in lines[1:1+count]:
    desc = var.split(' ')
    var_list.append({ 'name': desc[0], 'type': get_var_type(desc[1])})

  return var_list

def parse_response_type(response):
  if (not response_is_type(response)):
    return None

  return get_var_type(response.split('\n')[1])

def parse_response_get(response):
  if (not response_is_get(response)):
    return None
  
  desc = response.split('\n')[1].split(' ')
  var_name = desc[0]
  var_type = get_var_type(desc[1])

  if var_type is bool:
    return True if var_type != '0' else False
  return var_name, var_type(desc[2])