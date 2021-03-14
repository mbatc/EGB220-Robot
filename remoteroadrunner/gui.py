import imgui

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



class GUI:
  def __init__(self, app): 
    self.app = app
    self.create_windows()
    self.setup_style()

  def update(self):
    self.command_wnd.x      = self.app.window.width() * 1 / 4
    self.command_wnd.width  = self.app.window.width() * 3 / 8
    self.command_wnd.height = self.app.window.height() - self.console_height

    self.var_wnd.x      = self.app.window.width() * 5 / 8
    self.var_wnd.width  = self.app.window.width() * 3 / 8
    self.var_wnd.height = self.app.window.height() - self.console_height

    self.console.x      = self.app.window.width() / 4
    self.console.y      = self.app.window.height() - self.console_height
    self.console.width  = self.app.window.width() * 3 / 4
    self.console.height = self.console_height

    self.conn.width  = self.app.window.width() / 4
    self.conn.height = self.app.window.height()

  def draw(self):
    self.command_wnd.draw()
    self.var_wnd.draw()
    self.console.draw()
    self.conn.draw()

  def create_windows(self):
    self.console_height = 250
    self.command_wnd = CommandWindow(
      self,
      self.app.window.width() * 1 / 4, 0,
      self.app.window.width() * 3 / 8, self.app.window.height() - self.console_height
    )

    self.var_wnd = VariableWindow(
      self,
      self.app.window.width() * 5 / 8, 0,
      self.app.window.width() * 3 / 8, self.app.window.height() - self.console_height
    )

    self.console = ConsoleWindow(
      self,
      self.app.window.width() / 4, self.app.window.height() - self.console_height,
      self.app.window.width() * 3 / 4, self.console_height
    )

    self.conn = ConnectionWindow(
      self,
      0, 0,
      self.app.window.width() / 4, self.app.window.height()
    )
    
    wnd_flags = imgui.WINDOW_NO_RESIZE|imgui.WINDOW_NO_MOVE|imgui.WINDOW_NO_SCROLLBAR|imgui.WINDOW_NO_COLLAPSE
    self.command_wnd.add_flags(wnd_flags)
    self.var_wnd.add_flags    (wnd_flags)
    self.console.add_flags    (wnd_flags)
    self.conn.add_flags       (wnd_flags)

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
 