from __future__ import absolute_import

from autopilot.utilities import Silence

from Xlib import X, display, protocol

_display = None

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
