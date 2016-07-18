/*
 * Copyright (C) 2016 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <ccs.h>
#include <gio/gio.h>

#include <iostream>
#include <string>

extern const CCSInterfaceTable ccsDefaultInterfaceTable;

void PluginSetActiveWithDeps(CCSContext* context, std::string const& plugin_name)
{
  if (plugin_name.empty())
    return;

  auto plugin = ccsFindPlugin(context, plugin_name.c_str());

  if (!plugin)
    return;

  auto reqs = ccsPluginGetRequiresPlugins(plugin);
  for (auto req = reqs; req; req = req->next)
  {
    if (req->data && req->data->value)
    {
      std::string name = req->data->value;
      PluginSetActiveWithDeps(context, name.c_str());
    }
  }

  if (!ccsPluginIsActive(context, plugin_name.c_str())) {
    std::cout << "Setting plugin '" << plugin_name << "' to active" << std::endl;
    ccsPluginSetActive(plugin, true);
  }

  auto conflicts = ccsPluginGetConflictPlugins(plugin);
  for (auto con = conflicts; con; con = con->next)
  {
    if (con->data && con->data->value)
    {
      std::string name = con->data->value;
      auto plugin = ccsFindPlugin(context, name.c_str());

      if (ccsPluginIsActive(context, name.c_str()))
      {
        std::cout << "Setting plugin '" << name << "' to non-active" << std::endl;
        ccsPluginSetActive(plugin, false);
      }
    }
  }
}

int main()
{
  auto context = ccsContextNew (0, &ccsDefaultInterfaceTable);

  if (!context)
    return -1;

  PluginSetActiveWithDeps(context, "unityshell");

  ccsWriteChangedSettings(context);
  g_settings_sync();
  ccsFreeContext(context);

  return 0;
}
