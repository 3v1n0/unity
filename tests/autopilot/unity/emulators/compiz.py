from __future__ import absolute_import

"""Functions that wrap compizconfig to avoid some unpleasantness in that module."""

from __future__ import absolute_import

from autopilot.utilities import Silence

_global_context = None

def get_global_context():
    """Get the compizconfig global context object."""
    global _global_context
    if _global_context is None:
        with Silence():
            from compizconfig import Context
            _global_context = Context()
    return _global_context


def _get_plugin(plugin_name):
    """Get a compizconfig plugin with the specified name.

    Raises KeyError of the plugin named does not exist.

    """
    ctx = get_global_context()
    with Silence():
        try:
            return ctx.Plugins[plugin_name]
        except KeyError:
            raise KeyError("Compiz plugin '%s' does not exist." % (plugin_name))


def get_compiz_setting(plugin_name, setting_name):
    """Get a compiz setting object.

    *plugin_name* is the name of the plugin (e.g. 'core' or 'unityshell')
    *setting_name* is the name of the setting (e.g. 'alt_tab_timeout')

    :raises: KeyError if the plugin or setting named does not exist.

    :returns: a compiz setting object.

    """
    plugin = _get_plugin(plugin_name)
    with Silence():
        try:
            return plugin.Screen[setting_name]
        except KeyError:
            raise KeyError("Compiz setting '%s' does not exist in plugin '%s'." % (setting_name, plugin_name))


def get_compiz_option(plugin_name, setting_name):
    """Get a compiz setting value.

    This is the same as calling:

    >>> get_compiz_setting(plugin_name, setting_name).Value

    """
    return get_compiz_setting(plugin_name, setting_name).Value
