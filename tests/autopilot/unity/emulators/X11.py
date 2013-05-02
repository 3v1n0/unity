from __future__ import absolute_import

from autopilot.utilities import Silence
from autopilot.display import Display
from autopilot.input import Mouse, Keyboard
from autopilot.process import Window

import logging
import subprocess
from Xlib import X, display, protocol


logger = logging.getLogger(__name__)

_display = None

_blacklisted_drivers = ["NVIDIA"]

def _get_display():
    """Get a Xlib display object. Creating the display prints garbage to stdout."""
    global _display
    if _display is None:
        with Silence():
            _display = display.Display()
    return _display

def _getProperty(_type, win=None):
    if not win:
        win = _get_display().screen().root
    atom = win.get_full_property(_get_display().get_atom(_type), X.AnyPropertyType)
    if atom: return atom.value

def get_desktop_viewport():
    """Get the x,y coordinates for the current desktop viewport top-left corner."""
    return _getProperty('_NET_DESKTOP_VIEWPORT')


# Note: this use to exist within autopilot, moved here due to Autopilot 1.3
# upgrade.
def drag_window_to_screen(window, screen):
    """Drags *window* to *screen*

    :param autopilot.process.Window window: The window to drag
    :param integer screen: The screen to drag the *window* to
    :raises: **TypeError** if *window* is not a autopilot.process.Window

    """
    if not isinstance(window, Window):
        raise TypeError("Window must be a autopilot.process.Window")

    if window.monitor == screen:
        logger.debug("Window %r is already on screen %d." % (window.x_id, screen))
        return

    assert(not window.is_maximized)
    (win_x, win_y, win_w, win_h) = window.geometry
    (mx, my, mw, mh) = Display.create().get_screen_geometry(screen)

    logger.debug("Dragging window %r to screen %d." % (window.x_id, screen))

    mouse = Mouse.create()
    keyboard = Keyboard.create()
    mouse.move(win_x + win_w/2, win_y + win_h/2)
    keyboard.press("Alt")
    mouse.press()
    keyboard.release("Alt")

    # We do the movements in two steps, to reduce the risk of being
    # blocked by the pointer barrier
    target_x = mx + mw/2
    target_y = my + mh/2
    mouse.move(win_x, target_y, rate=20, time_between_events=0.005)
    mouse.move(target_x, target_y, rate=20, time_between_events=0.005)
    mouse.release()


# Note: this use to exist within autopilot, moved here due to Autopilot 1.3
# upgrade.
def set_primary_monitor(monitor):
    """Set *monitor* to be the primary monitor.

    :param int monitor: Must be between 0 and the number of configured
     monitors.
    :raises: **ValueError** if an invalid monitor is specified.
    :raises: **BlacklistedDriverError** if your video driver does not
     support this.

    """
    try:
        glxinfo_out = subprocess.check_output("glxinfo")
    except OSError, e:
        raise OSError("Failed to run glxinfo: %s. (do you have mesa-utils installed?)" % e)

    for dri in _blacklisted_drivers:
        if dri in glxinfo_out:
            raise Display.BlacklistedDriverError('Impossible change the primary monitor for the given driver')

    num_monitors = Display.create().get_num_screens()
    if monitor < 0 or monitor >= num_monitors:
        raise ValueError('Monitor %d is not in valid range of 0 <= monitor < %d.' % (num_monitors))

    default_screen = Gdk.Screen.get_default()
    monitor_name = default_screen.get_monitor_plug_name(monitor)

    if not monitor_name:
        raise ValueError('Could not get monitor name from monitor number %d.' % (monitor))

    ret = os.spawnlp(os.P_WAIT, "xrandr", "xrandr", "--output", monitor_name, "--primary")

    if ret != 0:
        raise RuntimeError('Xrandr can\'t set the primary monitor. error code: %d' % (ret))

def reset_display():
    from autopilot.input._X11 import reset_display
    reset_display()
