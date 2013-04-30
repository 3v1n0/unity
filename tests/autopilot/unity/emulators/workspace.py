# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2012 Canonical
# Author: Thomi Richards
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#

from __future__ import absolute_import

from autopilot.keybindings import KeybindingsHelper
from autopilot.display import Display

from unity.emulators.compiz import get_compiz_option
from unity.emulators.X11 import get_desktop_viewport


class WorkspaceManager(KeybindingsHelper):
    """Class to manage switching to different workspaces."""

    def __init__(self):
        super(WorkspaceManager, self).__init__()
        self.refresh_workspace_information()

    @property
    def num_workspaces(self):
        """The number of workspaces configured."""
        return self._workspaces_wide * self._workspaces_high

    @property
    def current_workspace(self):
        """The current workspace number. 0 <= x < num_workspaces."""
        vx,vy = get_desktop_viewport()
        return self._coordinates_to_vp_number(vx, vy)

    def refresh_workspace_information(self):
        """Re-read information about available workspaces from compiz and X11."""
        self._workspaces_wide = get_compiz_option("core", "hsize")
        self._workspaces_high = get_compiz_option("core", "vsize")
        # self._desktop_width, self.desktop_height = get_desktop_geometry()
        # i.e. 1440x900
        # Note: only gets the viewport for the first monitor.
        _, _, self._viewport_width, self._viewport_height = Display.create().get_screen_geometry(0)
        # self._viewport_width = self._desktop_width / self._workspaces_wide
        # self._viewport_height = self.desktop_height / self._workspaces_high

    def switch_to(self, workspace_num):
        """Switch to the workspace specified.

        ValueError is raised if workspace_num is outside 0<= workspace_num < num_workspaces.

        """
        if workspace_num < 0 or workspace_num >= self.num_workspaces:
            raise ValueError("Workspace number must be between 0 and %d" % self.num_workspaces)

        current_row, current_col = self._vp_number_to_row_col(self.current_workspace)
        target_row, target_col = self._vp_number_to_row_col(workspace_num)
        lefts = rights = ups = downs = 0
        if current_col > target_col:
            lefts = current_col - target_col
        else:
            rights = target_col - current_col
        if current_row > target_row:
            ups = current_row - target_row
        else:
            downs = target_row - current_row

        for i in range(lefts):
            self.keybinding("workspace/move_left")
        for i in range(rights):
            self.keybinding("workspace/move_right")
        for i in range(ups):
            self.keybinding("workspace/move_up")
        for i in range(downs):
            self.keybinding("workspace/move_down")

    def _coordinates_to_vp_number(self, vx, vy):
        """Translate viewport coordinates to a viewport number."""
        row,col = self._coordinates_to_row_col(vx, vy)
        return (row * self._workspaces_wide) + col

    def _coordinates_to_row_col(self, vx, vy):
        """Translate viewport coordinates to viewport row,col."""
        row = vy / self._viewport_height
        col = vx / self._viewport_width
        return (row,col)

    def _vp_number_to_row_col(self, vp_number):
        """Translate a viewport number to a viewport row/col."""
        row = vp_number / self._workspaces_wide
        col = vp_number % self._workspaces_wide
        return (row,col)
