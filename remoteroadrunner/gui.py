import math
import sys

import imgui


def to_rads(degs):
  return degs * math.pi / 180

def to_degs(rads):
  return rads * 180 / math.pi

class Vector2:
  def __init__(self, x, y):
    self.x = x
    self.y = y

  def add(self, vec):
    return Vector2(self.x + vec.x, self.y + vec.y)

  def sub(self, vec):
    return Vector2(self.x - vec.x, self.y - vec.y)

  def mul(self, vec):
    return Vector2(self.x * vec.x, self.y * vec.y)
    
  def div(self, vec):
    return Vector2(self.x / vec.x, self.y / vec.y)

  def scale(self, value):
    return Vector2(self.x * value, self.y * value)

  def dot(self, vec):
    return self.x * vec.x + self.y * vec.y

  def cross(self, vec):
    return self.x * self.y - self.y * vec.x

  def rotate(self, degs):
    '''
    Rotate this vector by the specified degrees
    '''
    cs = math.cos(to_rads(degs))
    sn = math.sin(to_rads(degs))
    px = self.x * cs - self.y * sn; 
    py = self.x * sn + self.y * cs;
    return Vector2(px, py)

  def mag(self):
    '''
    Get the magnitude of this vector
    '''
    return math.sqrt(self.x * self.x + self.y * self.y)

  def normalize(self):
    return self.scale(1 / self.mag())

  def angle(self, vec):
    '''
    Get the angle between two vectors
    '''
    return to_degs(math.acos(self.dot(vec) / (self.mag() * vec.mag())))

  def min(self, vec):
    return Vector2(min(self.x, vec.x), min(self.y, vec.y))

  def max(self, vec):
    return Vector2(max(self.x, vec.x), max(self.y, vec.y))

  def equals(self, vec):
    return self.x == vec.x and self.y == vec.y

  @staticmethod
  def largest():
    return Vector2(sys.float_info.max, sys.float_info.max)

  @staticmethod
  def smallest():
    return Vector2(sys.float_info.min, sys.float_info.min)

class Window:
  def __init__(self, ui, x, y, width, height, name):
    self.name   = name
    self.x      = x
    self.y      = y
    self.width  = width
    self.height = height
    self.ui     = ui
    self.app    = ui.app
    self.flags  = 0

  def add_flags(self, flags):
    self.flags = self.flags|flags

  def set_flags(self, flags):
    self.flags = flags

  def rem_flags(self, flags):
    self.flags = self.flags & ~flags

  def __begin(self):
    imgui.set_next_window_position(self.x, self.y)
    imgui.set_next_window_size    (self.width, self.height)
    imgui.begin(self.name, flags=self.flags)

  def __end(self):
    imgui.end()

  def on_draw(self):
    pass

  def draw(self):
    self.__begin()
    self.on_draw()
    self.__end()

class CommandWindow(Window):
  def __init__(self, ui, x, y, width, height):
    super(CommandWindow, self).__init__(ui, x, y, width, height, "Commands")

  def on_draw(self):
    style = imgui.get_style()
    imgui.begin_child("CommandList", 0, -20 - style.item_spacing.y * 2, True)
    for cmd_name in self.app.context.get_commands():
      clicked, _ = imgui.selectable(cmd_name)
      if clicked:
        self.app.call_command(cmd_name)

    imgui.end_child()
    
    width = imgui.get_window_content_region_width()
    if imgui.button("Refresh", width):
      self.app.refresh_commands()



class VariableWindow(Window):
  def __init__(self, ui, x, y, width, height):
    super(VariableWindow, self).__init__(ui, x, y, width, height, "Variables")

  def show_var(self, name):
    val = self.app.context.get_var(name)
    changed = False
    if isinstance(val, float):
      changed, new_val = imgui.input_float(name, val)
    elif isinstance(val, bool):
      changed, new_val = imgui.checkbox(name,    val)
    elif isinstance(val, int):
      changed, new_val = imgui.input_int(name,   val)
    else:
      imgui.text('Unknown: ' + name)

    if not imgui.is_item_active():
      self.app.context.set_var(name, new_val)

  def on_draw(self):
    style = imgui.get_style()
    imgui.begin_child("VarList", 0, -20 - style.item_spacing.y * 2, True)
    for var_name in self.app.context.get_variables():
      self.show_var(var_name)
    imgui.end_child()

    width = imgui.get_window_content_region_width()
    if imgui.button("Refresh", width):
      self.app.refresh_variables()



class ConnectionWindow(Window):
  def __init__(self, ui, x, y, width, height):
    super(ConnectionWindow, self).__init__(ui, x, y, width, height, "Device List")
  
  def on_draw(self):
    style = imgui.get_style()
    imgui.begin_child("DeviceList", 0, -20 - style.item_spacing.y * 2, True)
    for addr, device in self.app.devices.items():
      changed, other = imgui.selectable(device.detailed(), addr == self.app.connected_device)
      if changed:
        self.app.connect(addr)
    imgui.end_child()

    width = imgui.get_window_content_region_width()
    if self.app.is_scanning:
      if imgui.button("stop scanning", width):
        self.app.stop_scanner()
    else:
      if imgui.button("scan for devices", width):
        self.app.start_scanner()

class ConsoleWindow(Window):
  def __init__(self, ui, x, y, width, height, take_focus=True):
    super(ConsoleWindow, self).__init__(ui, x, y, width, height, "Console")
    self.input = ""
    self.history = []
    self.history_idx = -1
    self.last_history = -1
    self.auto_scroll = True
    self.last_log_count = 0
    self.grab_focus = take_focus
  
  def take_focus(self):
    self.grab_focus = True

  def on_draw(self):
    style = imgui.get_style()
    imgui.begin_child("ConsoleLog", 0, -20 - imgui.get_text_line_height_with_spacing() - style.item_spacing.y * 2, True)
    # Draw all the log entries
    for entry in self.app.console_log:
      # Each log consists of multiple parts, so that bits can be coloured differently
      for part in entry:
        # Try apply the colour
        has_col = "col" in part and part["col"] != None
        if has_col:
          imgui.push_style_color(imgui.COLOR_TEXT, *part["col"])

        # Draw the text
        imgui.text(part["text"])

        # Remove the color change
        if has_col:
          imgui.pop_style_color()
        imgui.same_line()
      imgui.new_line()
          

    log_count = len(self.app.console_log)
    if self.auto_scroll and self.last_log_count != log_count:
      imgui.set_scroll_here()
    self.last_log_count = log_count

    imgui.end_child()
    
    self.history_idx = max(0, min(self.history_idx, len(self.history) - 1))
    if self.last_history != self.history_idx and self.history_idx < len(self.history):
      # Force the input box to loose focus so we can change the text
      last_cursor = imgui.get_cursor_pos()
      imgui.set_cursor_screen_pos(-100, -100)
      imgui.input_text('dummy', "", 64)
      imgui.set_keyboard_focus_here()

      # Change the text to something from the history
      self.input = self.history[self.history_idx]

      # Re-grab focus
      self.take_focus()

    changed, self.input = imgui.input_text("Input", self.input, 128, imgui.INPUT_TEXT_ENTER_RETURNS_TRUE)

    # Grab focus if we need to
    if self.grab_focus:
      imgui.set_keyboard_focus_here()
    self.grab_focus = False

    # Logic to select command history
    self.last_history = self.history_idx
    if imgui.is_item_active() and imgui.get_io().keys_down[imgui.KEY_UP_ARROW]:
      self.history_idx = self.history_idx - 1
    if imgui.is_item_active() and imgui.get_io().keys_down[imgui.KEY_DOWN_ARROW]:
      self.history_idx = self.history_idx + 1

    # Draw auto scroll option
    imgui.same_line()
    _, self.auto_scroll = imgui.checkbox("Auto Scroll", self.auto_scroll)

    if changed:
      self.app.console_in.append(self.input)

      if len(self.input) > 0:
        if len(self.history) > 0:
          self.history.pop()
        self.history.append(self.input)
        self.history.append("")
        self.input = ""
        self.take_focus()

STRAIGHT = 0
RTURN    = 1
LTURN    = 2

TRACK_TYPE_NAME = [
  'Straight',
  'Right Turn',
  'Left Turn'
]

SEGMENT_LINE = 0
SEGMENT_ARC  = 1

def sign(x):
  return -1 if x < 0 else 1

class Segment:
  def __init__(self, type, params):
    self.type  = type
    self.params = params

  def draw(self, draw_list, origin, offset, scale, is_bg):
    if (self.type == SEGMENT_LINE):
      self.draw_line(draw_list, origin, offset, scale, is_bg)
    elif (self.type == SEGMENT_ARC):
      self.draw_arc(draw_list, origin, offset, scale, is_bg)

  def draw_line(self, draw_list, origin, offset, scale, is_bg):
    start = self.params['start']
    end   = self.params['end']
    col   = self.params['colour'] if 'colour' in self.params else (1, 1, 1, 1)

    start = start.sub(origin).scale(scale).add(offset)
    end   = end.sub(origin).scale(scale).add(offset)

    if is_bg:
      draw_list.add_line(start.x, start.y, end.x, end.y, imgui.get_color_u32_rgba(0, 0, 0, 1), scale)
    else:
      draw_list.add_line(start.x, start.y, end.x, end.y, imgui.get_color_u32_rgba(col[0], col[1], col[2], col[3]), 0.1 * scale)

  def draw_arc(self, draw_list, origin, offset, scale, is_bg):
    start     = self.params['start']  # The start point of the arg
    center    = self.params['center'] # The center of the arc
    arc_angle = self.params['arc']   # The size of the arg in degrees
    col       = self.params['colour'] if 'colour' in self.params else (1, 1, 1, 1) # The colour of the arc

    # Calculate how many line segments to draw the arc with
    num_segments = math.ceil(max(abs(arc_angle) / 5, 1))

    # Calculate the angular size of each segment
    step = arc_angle / num_segments

    # Get the vector from the center to the start position.
    # We will rotate this vector to get each position.
    rot_arm = start.sub(center)

    marker_start = start.sub(rot_arm.normalize().scale(0.1)).sub(origin).scale(scale).add(offset)
    marker_end   = start.sub(rot_arm.normalize().scale(0.2)).sub(origin).scale(scale).add(offset)

    # Calculate points along the segment
    points  = [ start.sub(origin).scale(scale).add(offset) ]
    start_tangent = rot_arm.rotate(90).normalize().scale(sign(arc_angle))
    for i in range(num_segments):
      rot_arm = rot_arm.rotate(step) # Rotate our offset vector
      seg_end = center.add(rot_arm) # The end of the segment is the center + the rotated offset vector
      points.append(seg_end.sub(origin).scale(scale).add(offset))
    end_tangent = rot_arm.rotate(90).normalize().scale(sign(arc_angle))
    points.insert(0, points[0].sub(start_tangent))
    points.append(points[-1].add(end_tangent))

    end_marker_start = points[-1].sub(rot_arm.normalize().scale(0.1 * scale))
    end_marker_end   = points[-1].sub(rot_arm.normalize().scale(0.2 * scale))

    # Convert to imgui vec2's      
    points = [ imgui.Vec2(a.x, a.y) for a in points ]

    # Draw the curve
    if is_bg:
      draw_list.add_polyline(
        points,
        imgui.get_color_u32_rgba(0,0,0,1), # Colour
        False, scale)
    else:
      u32Col = imgui.get_color_u32_rgba(col[0], col[1], col[2], col[3])

      draw_list.add_polyline(
        points,
        u32Col,  # Colour
        False, 0.1 * scale)
      
      draw_list.add_line(
        marker_start.x, marker_start.y,
        marker_end.x, marker_end.y,
        u32Col,  # Colour
        0.1 * scale)

      draw_list.add_line(
        end_marker_start.x, end_marker_start.y,
        end_marker_end.x,   end_marker_end.y,
        u32Col,  # Colour
        0.1 * scale)

class BoundingBox():
  def __init__(self, min = Vector2.largest(), max = Vector2.smallest()):
    self.min = min
    self.max = max

  def contains(self, point):
    clamped = point.max(self.min).min(self.max)
    return clamped.equals(point)

  def grow(self, point):
    self.max = point.max(self.max)
    self.min = point.min(self.min)

  def center(self):
    return (self.max.add(self.min)).scale(0.5)

  def size(self):
    return self.max.sub(self.min)

  def longest_edge(self):
    return max(self.size().x, self.size().y)
    

class TrackMapWindow(Window):
  def __init__(self, ui, x, y, width, height):
    super(TrackMapWindow, self).__init__(ui, x, y, width, height, "Track Map")
    self.track_details = [
      [STRAIGHT, 2],
      [RTURN,    2],
      [STRAIGHT, 2],
      [RTURN,    2],
      [STRAIGHT, 2],
      [RTURN,    2],
      [STRAIGHT, 2],
      [RTURN,    2]
    ]

    self.lap_times = [
      10.0,
      9.0
    ]

    self.hovered_track  = -1
    self.selected_track = -1

  def on_draw(self):
    # Get the draw list so we can do some custom drawing
    pos    = imgui.get_window_position()
    size   = imgui.get_window_size()
    center = imgui.Vec2(pos.x + size.x / 2, pos.y + size.y / 2)

    imgui.begin_child('map-preview', 0, size.y * 0.7)

    track_pos    = imgui.get_window_position()
    track_size   = imgui.get_window_size()
    track_center = imgui.Vec2(track_pos.x + track_size.x / 2, track_pos.y + track_size.y / 2)

    self.draw_track_map(
      imgui.get_window_draw_list(),
      self.track_details,
      Vector2(track_center.x, track_center.y),
      min(track_size.x * 0.8, track_size.y * 0.8)
    )

    imgui.end_child()

    imgui.new_line()
    
    imgui.separator()
    imgui.columns(2)
    imgui.text('Lap Times')
    imgui.begin_child('lap-times', 0.5, 0, True)
    counter = 1
    imgui.columns(2)
    for time in self.lap_times:
      imgui.text(str(counter))
      imgui.next_column()
      imgui.text(str(time) + 's')
      imgui.next_column()
      counter = counter + 1
    imgui.columns(1)
    imgui.end_child()

    imgui.next_column()
    
    imgui.text('Track Details')
    imgui.begin_child('track-segments', 0.5, 0, True)
    imgui.columns(2)

    self.hovered_track = -1
    for i, detail in enumerate(self.track_details):
      imgui.push_id(str(i))
      start_pos = imgui.get_cursor_screen_pos()

      if imgui.selectable(str(i + 1))[1]:
        self.selected_track = i if self.selected_track != i else -1
      hovered = imgui.is_item_hovered()

      imgui.next_column()

      _, detail[0] = imgui.listbox(TRACK_TYPE_NAME[detail[0]], detail[0], TRACK_TYPE_NAME, 1)
      _, detail[1] = imgui.input_float('size', detail[1])
      if imgui.button('Remove'):
        self.track_details.remove(detail)
      end_pos = imgui.get_cursor_screen_pos()
      max_x   = imgui.get_window_position().x + imgui.get_window_size().x
      imgui.pop_id()

      hovered = imgui.is_mouse_hovering_rect(start_pos.x, start_pos.y, max_x, end_pos.y)
      imgui.next_column()
      hovered |= imgui.is_mouse_hovering_rect(start_pos.x, start_pos.y, max_x, end_pos.y)
      if hovered:
        self.hovered_track = i
      imgui.separator()

    if imgui.button('Add'):
      self.track_details.append([ STRAIGHT, 1 ])
    imgui.columns(1)
    imgui.end_child()

  def draw_track_map(self, draw_list, map_sections, map_center, map_size):
    # Determine points for the map
    segments   = []           
    last_point = Vector2(0, 0) # Start at 0,0 (we will recenter later)
    dir        = Vector2(1, 0) # Start facing right

    map_bounds = BoundingBox()
    map_bounds.grow(last_point)

    for i, section in enumerate(map_sections):
      if section[0] == STRAIGHT:

        next_point = last_point.add(dir.scale(section[1]))

        segments.append(Segment(SEGMENT_LINE, {
          'start': last_point,
          'end':   next_point,
        }))

      elif section[0] == RTURN:
        dir = dir.rotate(90)
        next_point = last_point.add(dir)
        center     = last_point.add(dir.scale(section[1]))
        next_point = center.add(last_point.sub(center).rotate(90))
        segments.append(Segment(SEGMENT_ARC, {
          'start':  last_point,
          'center': center,
          'arc':    90
        }))

      elif section[0] == LTURN:
        dir        = dir.rotate(-90)
        center     = last_point.add(dir.scale(section[1]))
        next_point = center.add(last_point.sub(center).rotate(-90))

        segments.append(Segment(SEGMENT_ARC, {
          'start':  last_point,
          'center': center,
          'arc':    -90
        }))

      else:
        continue
      
      if i == self.hovered_track:
        segments[-1].params['colour'] = (0, 0, 1, 1)
      if i == self.selected_track:
        segments[-1].params['colour'] = (0, 1, 0, 1)

      last_point = next_point
      map_bounds.grow(next_point)

    # Calculate an offset and scale for the sections so we draw the map at
    # the request position and scale
    draw_offset = map_center.sub(map_bounds.center())
    draw_scale  = map_size / map_bounds.longest_edge()

    # Draw the line background first
    for seg in segments: seg.draw(draw_list, map_bounds.center(), draw_offset, draw_scale, True)
    # Draw the actual line
    for seg in segments: seg.draw(draw_list, map_bounds.center(), draw_offset, draw_scale, False)


class GUI:
  def __init__(self, app): 
    self.app = app
    self.create_windows()
    self.setup_style()

  def update(self):
    self.var_wnd.x      = self.app.window.width() * 6 / 10
    self.var_wnd.width  = self.app.window.width() * 4 / 10
    self.var_wnd.height = (self.app.window.height() - self.console_height) / 2

    self.command_wnd.x      = self.var_wnd.x
    self.command_wnd.y      = self.var_wnd.height
    self.command_wnd.width  = self.app.window.width() * 4 / 10
    self.command_wnd.height = self.var_wnd.height

    self.console.x      = self.app.window.width() / 5
    self.console.y      = self.app.window.height() - self.console_height
    self.console.width  = self.app.window.width() * 8 / 10
    self.console.height = self.console_height

    self.conn.width  = self.app.window.width() / 5
    self.conn.height = self.app.window.height()

    self.map.x      = self.app.window.width() * 1 / 5
    self.map.y      = 0
    self.map.width  = self.app.window.width() * 4 / 10
    self.map.height = self.app.window.height() - self.console_height

  def draw(self):
    self.command_wnd.draw()
    self.var_wnd.draw()
    self.console.draw()
    self.conn.draw()
    self.map.draw()

  def create_windows(self):
    self.console_height = 250
    self.command_wnd = CommandWindow(
      self,
      self.app.window.width() * 1 / 5, 0,
      self.app.window.width() * 4 / 10, self.app.window.height() - self.console_height
    )

    self.var_wnd = VariableWindow(
      self,
      self.app.window.width() * 5 / 8, 0,
      self.app.window.width() * 4 / 10, self.app.window.height() - self.console_height
    )

    self.console = ConsoleWindow(
      self,
      self.app.window.width() / 5, self.app.window.height() - self.console_height,
      self.app.window.width() * 4 / 5, self.console_height
    )

    self.conn = ConnectionWindow(
      self,
      0, 0,
      self.app.window.width() / 5, self.app.window.height()
    )

    self.map = TrackMapWindow(
      self,
      self.app.window.width() * 1 / 5, 0,
      self.app.window.width() * 4 / 10, self.app.window.height() - self.console_height
    )
    
    wnd_flags = imgui.WINDOW_NO_RESIZE|imgui.WINDOW_NO_MOVE|imgui.WINDOW_NO_SCROLLBAR|imgui.WINDOW_NO_COLLAPSE
    self.command_wnd.add_flags(wnd_flags)
    self.var_wnd.add_flags    (wnd_flags)
    self.console.add_flags    (wnd_flags)
    self.conn.add_flags       (wnd_flags)
    self.map.add_flags        (wnd_flags)

  def setup_style(self):
    is_3d = True
    imgui.push_style_var(imgui.STYLE_WINDOW_ROUNDING, 0)
    imgui.push_style_color(imgui.COLOR_TEXT, 1, 1, 1, 1)
    imgui.push_style_color(imgui.COLOR_TEXT_DISABLED, 0.40, 0.40, 0.40, 1.00)
    imgui.push_style_color(imgui.COLOR_CHILD_BACKGROUND, 0.25, 0.25, 0.25, 1.00)
    imgui.push_style_color(imgui.COLOR_WINDOW_BACKGROUND, 0.25, 0.25, 0.25, 1.00)
    imgui.push_style_color(imgui.COLOR_POPUP_BACKGROUND, 0.25, 0.25, 0.25, 1.00)
    imgui.push_style_color(imgui.COLOR_BORDER, 0.12, 0.12, 0.12, 0.71)
    imgui.push_style_color(imgui.COLOR_BORDER_SHADOW, 1.00, 1.00, 1.00, 0.06)
    imgui.push_style_color(imgui.COLOR_FRAME_BACKGROUND, 0.42, 0.42, 0.42, 0.54)
    imgui.push_style_color(imgui.COLOR_FRAME_BACKGROUND_HOVERED, 0.42, 0.42, 0.42, 0.40)
    imgui.push_style_color(imgui.COLOR_FRAME_BACKGROUND_ACTIVE, 0.56, 0.56, 0.56, 0.67)
    imgui.push_style_color(imgui.COLOR_TITLE_BACKGROUND, 0.19, 0.19, 0.19, 1.00)
    imgui.push_style_color(imgui.COLOR_TITLE_BACKGROUND_ACTIVE, 0.22, 0.22, 0.22, 1.00)
    imgui.push_style_color(imgui.COLOR_TITLE_BACKGROUND_COLLAPSED, 0.17, 0.17, 0.17, 0.90)
    imgui.push_style_color(imgui.COLOR_MENUBAR_BACKGROUND, 0.335, 0.335, 0.335, 1.000)
    imgui.push_style_color(imgui.COLOR_SCROLLBAR_BACKGROUND, 0.24, 0.24, 0.24, 0.53)
    imgui.push_style_color(imgui.COLOR_SCROLLBAR_GRAB, 0.41, 0.41, 0.41, 1.00)
    imgui.push_style_color(imgui.COLOR_SCROLLBAR_GRAB_HOVERED, 0.52, 0.52, 0.52, 1.00)
    imgui.push_style_color(imgui.COLOR_SCROLLBAR_GRAB_ACTIVE, 0.76, 0.76, 0.76, 1.00)
    imgui.push_style_color(imgui.COLOR_CHECK_MARK, 0.65, 0.65, 0.65, 1.00)
    imgui.push_style_color(imgui.COLOR_SLIDER_GRAB, 0.52, 0.52, 0.52, 1.00)
    imgui.push_style_color(imgui.COLOR_SLIDER_GRAB_ACTIVE, 0.64, 0.64, 0.64, 1.00)
    imgui.push_style_color(imgui.COLOR_BUTTON, 0.54, 0.54, 0.54, 0.35)
    imgui.push_style_color(imgui.COLOR_BUTTON_HOVERED, 0.52, 0.52, 0.52, 0.59)
    imgui.push_style_color(imgui.COLOR_BUTTON_ACTIVE, 0.76, 0.76, 0.76, 1.00)
    imgui.push_style_color(imgui.COLOR_HEADER, 0.38, 0.38, 0.38, 1.00)
    imgui.push_style_color(imgui.COLOR_HEADER_HOVERED, 0.47, 0.47, 0.47, 1.00)
    imgui.push_style_color(imgui.COLOR_HEADER_ACTIVE, 0.76, 0.76, 0.76, 0.77)
    imgui.push_style_color(imgui.COLOR_SEPARATOR, 0.000, 0.000, 0.000, 0.137)
    imgui.push_style_color(imgui.COLOR_SEPARATOR_HOVERED, 0.700, 0.671, 0.600, 0.290)
    imgui.push_style_color(imgui.COLOR_SEPARATOR_ACTIVE, 0.702, 0.671, 0.600, 0.674)
    imgui.push_style_color(imgui.COLOR_RESIZE_GRIP, 0.26, 0.59, 0.98, 0.25)
    imgui.push_style_color(imgui.COLOR_RESIZE_GRIP_HOVERED, 0.26, 0.59, 0.98, 0.67)
    imgui.push_style_color(imgui.COLOR_RESIZE_GRIP_ACTIVE, 0.26, 0.59, 0.98, 0.95)
    imgui.push_style_color(imgui.COLOR_PLOT_LINES, 0.61, 0.61, 0.61, 1.00)
    imgui.push_style_color(imgui.COLOR_PLOT_LINES_HOVERED, 1.00, 0.43, 0.35, 1.00)
    imgui.push_style_color(imgui.COLOR_PLOT_HISTOGRAM, 0.90, 0.70, 0.00, 1.00)
    imgui.push_style_color(imgui.COLOR_PLOT_HISTOGRAM_HOVERED, 1.00, 0.60, 0.00, 1.00)
    imgui.push_style_color(imgui.COLOR_TEXT_SELECTED_BACKGROUND, 0.73, 0.73, 0.73, 0.35)
    imgui.push_style_color(imgui.COLOR_MODAL_WINDOW_DIM_BACKGROUND, 0.80, 0.80, 0.80, 0.35)
    imgui.push_style_color(imgui.COLOR_DRAG_DROP_TARGET, 1.00, 1.00, 0.00, 0.90)
    imgui.push_style_color(imgui.COLOR_NAV_HIGHLIGHT, 0.26, 0.59, 0.98, 1.00)
    imgui.push_style_color(imgui.COLOR_NAV_WINDOWING_HIGHLIGHT, 1.00, 1.00, 1.00, 0.70)
    imgui.push_style_color(imgui.COLOR_NAV_WINDOWING_DIM_BACKGROUND, 0.80, 0.80, 0.80, 0.20)

    imgui.push_style_var(imgui.STYLE_POPUP_ROUNDING, 3)
    imgui.push_style_var(imgui.STYLE_WINDOW_PADDING, imgui.Vec2(4, 4))
    imgui.push_style_var(imgui.STYLE_FRAME_PADDING, imgui.Vec2(6, 4))
    imgui.push_style_var(imgui.STYLE_ITEM_SPACING, imgui.Vec2(6, 2))

    imgui.push_style_var(imgui.STYLE_SCROLLBAR_SIZE, 18)

    imgui.push_style_var(imgui.STYLE_WINDOW_BORDERSIZE, 1)
    imgui.push_style_var(imgui.STYLE_CHILD_BORDERSIZE, 1)
    imgui.push_style_var(imgui.STYLE_POPUP_BORDERSIZE, 1)
    imgui.push_style_var(imgui.STYLE_FRAME_BORDERSIZE, 1 if is_3d else 0)

    imgui.push_style_var(imgui.STYLE_WINDOW_ROUNDING, 0)
    imgui.push_style_var(imgui.STYLE_CHILD_ROUNDING, 3)
    imgui.push_style_var(imgui.STYLE_FRAME_ROUNDING, 3)
    imgui.push_style_var(imgui.STYLE_SCROLLBAR_ROUNDING, 2)
    imgui.push_style_var(imgui.STYLE_GRAB_ROUNDING, 3)
 