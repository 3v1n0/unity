# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

import os.path
from testtools.matchers import Not, Is, Contains
from xdg.DesktopEntry import DesktopEntry

from autopilot.emulators.unity.quicklist import QuicklistMenuItemLabel
from autopilot.tests import AutopilotTestCase


class QuicklistActionTests(AutopilotTestCase):
    """Tests for quicklist actions."""

    scenarios = [
        ('remmina', {'app_name': 'Remmina'}),
    ]

    def test_quicklist_actions(self):
        """Test that all actions present in the destop file are shown in the quicklist."""
        self.start_app(self.app_name)

        # load the desktop file from disk:
        desktop_file = os.path.join('/usr/share/applications',
            self.KNOWN_APPS[self.app_name]['desktop-file']
            )
        de = DesktopEntry(desktop_file)
        # get the launcher icon from the launcher:
        launcher_icon = self.launcher.model.get_icon_by_tooltip_text(de.getName())
        self.assertThat(launcher_icon, Not(Is(None)))

        # open the icon quicklist, and get all the text labels:
        launcher = self.launcher.get_launcher_for_monitor(0)
        launcher.click_launcher_icon(launcher_icon, button=3)
        ql = launcher_icon.get_quicklist()
        ql_item_texts = [i.text for i in ql.items if type(i) is QuicklistMenuItemLabel]

        # iterate over all the actions from the desktop file, make sure they're
        # present in the quicklist texts.
        actions = de.getActions()
        for action in actions:
            key = 'Desktop Action ' + action
            self.assertThat(de.content, Contains(key))
            name = de.content[key]['Name']
            self.assertThat(ql_item_texts, Contains(name))


