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

#include <glib.h>

class Place;

class PlaceEntrySection
{
public:
  // As this class is to hide the implementation of the PlaceEntry, and will often be
  // hiding a more C-like API, the decision taken is to make all these stack allocated
  // and to discourage the views or controllers from saving references to these.
  virtual const char* GetName() const = 0;
  virtual const char* GetIcon() const = 0;
};

class PlaceEntryGroup
{
public:
  // As this class is to hide the implementation of the PlaceEntry, and will often be
  // hiding a more C-like API, the decision taken is to make all these stack allocated
  // and to discourage the views or controllers from saving references to these. Instead
  // please use GetId(), which will return a pointer that you can guarentee will be valid
  // and you can use to do lookups to views.
  virtual const void* GetId() const = 0;
  virtual const char* GetRenderer() const = 0;
  virtual const char* GetName() const = 0;
  virtual const char* GetIcon() const = 0;
};

class PlaceEntryResult
{
public:
  // As this class is to hide the implementation of the PlaceEntry, and will often be
  // hiding a more C-like API, the decision taken is to make all these stack allocated
  // and to discourage the views or controllers from saving references to these. Instead
  // please use GetId(), which will return a pointer that you can guarentee will be valid
  // and you can use to do lookups to views.
  virtual const void* GetId() const = 0;
  virtual const char* GetName() const = 0;
  virtual const char* GetIcon() const = 0;
  virtual const char* GetMimeType() const = 0;
  virtual const char* GetURI() const = 0;
  virtual const char* GetComment() const = 0;
};

class PlaceEntry : public sigc::trackable
{
public:

  typedef sigc::slot<void, PlaceEntry*, PlaceEntrySection&> SectionForeachCallback;
  typedef sigc::slot<void, PlaceEntry*, PlaceEntryGroup&>  GroupForeachCallback;
  typedef sigc::slot<void, PlaceEntry*, PlaceEntryGroup&, PlaceEntryResult&> ResultForeachCallback;

  virtual Place* GetParent() = 0;

  virtual const char* GetId() = 0;
  virtual const char* GetName() = 0;
  virtual const char* GetIcon() = 0;
  virtual const char* GetDescription() = 0;
  virtual const char* GetSearchHint() = 0;
  virtual guint64      GetShortcut() = 0;

  // For ordering entries within a place
  virtual guint32        GetPosition() = 0;

  // For DND, what can this entry handle
  virtual const char** GetMimetypes() = 0;

  virtual const std::map<char*, char*>& GetHints() = 0;

  // Whether the entry is sensitive to input (clicks/DND)
  virtual bool IsSensitive() = 0;

  // This is not really useful for views
  virtual bool IsActive() = 0;

  // Show this entry in the launcher
  virtual bool ShowInLauncher() = 0;

  // Include as part of global search results
  virtual bool ShowInGlobal() = 0;

  // Important to call this when the view is active/inactive, so the place can reset itself
  // if necessary
  virtual void SetActive(bool is_active) = 0;

  virtual void SetSearch(const char* search, std::map<char*, char*>& hints) = 0;
  virtual void SetActiveSection(guint32 section_id) = 0;
  virtual void SetGlobalSearch(const char* search, std::map<char*, char*>& hints) = 0;

  virtual void ForeachSection(SectionForeachCallback slot) = 0;

  virtual void ForeachGroup(GroupForeachCallback slot) = 0;
  virtual void ForeachResult(ResultForeachCallback slot) = 0;

  virtual void ForeachGlobalGroup(GroupForeachCallback slot) = 0;
  virtual void ForeachGlobalResult(ResultForeachCallback slot) = 0;

  virtual void GetResult(const void* id, ResultForeachCallback slot) = 0;
  virtual void GetGlobalResult(const void* id, ResultForeachCallback slot) = 0;

  virtual void ActivateResult(const void* id) = 0;
  virtual void ActivateGlobalResult(const void* id) = 0;

  // Signals

  sigc::signal<void, bool>                    active_changed;

  // This covers: name, icon and description properties
  sigc::signal<void>                          state_changed;

  sigc::signal<void, guint32>                 position_changed;
  sigc::signal<void, const char**>          mimetypes_changed;
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
  sigc::signal<void, PlaceEntry*>            global_renderer_changed;

  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&>                    group_added;
  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&, PlaceEntryResult&> result_added;
  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&, PlaceEntryResult&> result_removed;

  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&>                    global_group_added;
  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&, PlaceEntryResult&> global_result_added;
  sigc::signal<void, PlaceEntry*, PlaceEntryGroup&, PlaceEntryResult&> global_result_removed;

  sigc::signal<void, const char*, guint32, std::map<const char*, const char*>&> search_finished;
};

#endif // PLACE_ENTRY_H
