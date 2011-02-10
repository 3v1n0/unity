/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */
#ifndef PLACE_ENTRY_H
#define PLACE_ENTRY_H

#include <map>
#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include <dee.h>

class PlaceEntry : public sigc::trackable
{
public:

  enum
  {
    GROUP_RENDERER,
    GROUP_NAME,
    GROUP_ICON
  };

  enum
  {
    RESULT_URI,
    RESULT_ICON,
    RESULT_GROUP_ID,
    RESULT_MIMETYPE,
    RESULT_NAME,
    RESULT_COMMENT
  };

  virtual const gchar * GetId          () = 0;
  virtual const gchar * GetName        () = 0;
  virtual const gchar * GetIcon        () = 0;
  virtual const gchar * GetDescription () = 0;

  // For ordering entries within a place
  virtual guint32        GetPosition  () = 0;

  // For DND, what can this entry handle
  virtual const gchar ** GetMimetypes () = 0;

  virtual const std::map<gchar *, gchar *>& GetHints () = 0;

  // Whether the entry is sensitive to input (clicks/DND)
  virtual bool IsSensitive () = 0;

  // This is not really useful for views
  virtual bool IsActive    () = 0;

  // Show this entry in the launcher
  virtual bool ShowInLauncher () = 0;

  // Include as part of global search results
  virtual bool ShowInGlobal () = 0;

  // Important to call this when the view is active/inactive, so the place can reset itself
  // if necessary
  virtual void SetActive        (bool is_active) = 0;

  virtual void SetSearch        (const gchar *search, std::map<gchar*, gchar*>& hints) = 0;
  virtual void SetActiveSection (guint32 section_id) = 0;
  virtual void SetGlobalSearch  (const gchar *search, std::map<gchar*, gchar*>& hints) = 0;

  virtual DeeModel * GetSectionsModel () = 0;

  virtual DeeModel * GetGroupsModel () = 0;
  virtual DeeModel * GetResultsModel () = 0;

  virtual DeeModel * GetGlobalResultsModel () = 0;
  // Signals

  sigc::signal<void, bool>                    active_changed;

  // This covers: name, icon and description properties
  sigc::signal<void>                          state_changed;

  sigc::signal<void, guint32>                 position_changed;
  sigc::signal<void, const gchar **>          mimetypes_changed;
  sigc::signal<void, bool>                    sensitive_changed;

  // If ShowInLauncher or ShowInGlobal changes
  sigc::signal<void>                          visibility_changed;

  // We don't use this too much right now
  sigc::signal<void>                          hints_changed;
 
  // Should be very rare
  sigc::signal<void>                          sections_model_changed;

  // Definitely connect to this, as your setting sections the places might want
  // to update it's views accordingly
  sigc::signal<void>                          entry_renderer_changed;
  
  // This is not important outside of a global search aggregator
  sigc::signal<void>                          global_renderer_changed;
};

#endif // PLACE_ENTRY_H
