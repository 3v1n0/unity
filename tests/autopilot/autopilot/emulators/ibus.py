# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"Functions to deal with ibus service."

# without this the 'import ibus' imports us, and import ibus.common fails. 0.O
from __future__ import absolute_import
import ibus
import ibus.common
import os
import logging
from time import sleep

logger = logging.getLogger(__name__)


def get_ibus_bus():
    """Get the ibus bus object, possibly starting the ibus daemon if it's
    not already running.

    """
    max_tries = 5
    for i in range(max_tries):
        if ibus.common.get_address() is None:
            pid = os.spawnlp(os.P_NOWAIT, "ibus-daemon", "ibus-daemon", "-d", "--xim")
            logger.info("Started ibus-daemon with pid %i." % (pid))
            sleep(2)
        else:
            return ibus.Bus()
    raise RuntimeError("Could not start ibus-daemon after %d tries." % (max_tries))


def get_available_input_engines():
    """Get a list of available input engines."""
    bus = get_ibus_bus()
    return [e.name for e in bus.list_engines()]


def get_active_input_engines():
    """Get the list of input engines that have been activated."""
    bus = get_ibus_bus()
    return [e.name for e in bus.list_active_engines()]


def set_global_input_engine(engine_name):
    """Set the global iBus input engine by name.

    This function enables the global input engine. To turn it off again, pass None
    as the engine name.

    """
    if not (engine_name is None or isinstance(engine_name, basestring)):
        raise TypeError("engine_name type must be either str or None.")

    bus = get_ibus_bus()

    if engine_name:
        available_engines = get_available_input_engines()
        if not engine_name in available_engines:
            raise ValueError("Unknown engine '%s'" % (engine_name))
        bus.get_config().set_value("general", "use_global_engine", True)
        bus.set_global_engine(engine_name)
        logger.info('Enabling global ibus engine "%s".' % (engine_name))
    else:
        bus.get_config().set_value("general", "use_global_engine", False)
        logger.info('Disabling global ibus engine.')
