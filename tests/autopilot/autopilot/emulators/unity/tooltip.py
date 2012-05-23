# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Andrea Azzarone
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

import logging
from time import sleep

from autopilot.emulators.unity import UnityIntrospectionObject


logger = logging.getLogger(__name__)


class ToolTip(UnityIntrospectionObject):
    """Represents a tooltip."""
