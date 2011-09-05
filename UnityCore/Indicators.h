// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITY_INDICATORS_H
#define UNITY_INDICATORS_H

#include <sigc++/trackable.h>

#include "Indicator.h"


namespace unity
{
namespace indicator
{

class Indicators : public sigc::trackable, boost::noncopyable
{
public:
  Indicators();
  virtual ~Indicators();

  /**
   * internal
   */
  void ActivateEntry(std::string const& entry_id);

  /**
   * internal
   */
  void SetEntryShowNow(std::string const& entry_id, bool show_now);

  /**
   * internal
   */
  virtual void OnEntryScroll(std::string const& entry_id, int delta) = 0;

  /**
   * internal
   */
  virtual void OnEntryShowMenu(std::string const& entry_id,
                               int x, int y, int timestamp, int button) = 0;
  virtual void OnEntrySecondaryActivate(std::string const& entry_id,
                                        unsigned int timestamp) = 0;

  // Signals
  sigc::signal<void, Indicator::Ptr const&> on_object_added;
  sigc::signal<void, Indicator::Ptr const&> on_object_removed;

  /**
   * Service wants the view to activate an entry.
   * Example use-case: user has activated an entry with the mouse and pressed
   * Left or Right key to activate previous or next entry.
   * @param entry_id entry id
   */
  sigc::signal<void, std::string const&> on_entry_activate_request;

  /**
   * An entry just got activated. View needs to repaint it.
   * @param entry_id entry id
   */
  sigc::signal<void, std::string const&> on_entry_activated;

  /**
   * internal
   */
  sigc::signal<void> on_synced;

  /**
   * The service is about to show a menu.
   * @param entry_id entry id
   * @param x x coordinate
   * @param y y coordinate
   * @param timestamp current time
   * @param button pressed button
   */
  sigc::signal<void, std::string const&, int, int, int, int> on_entry_show_menu;

protected:
  Indicator::Ptr GetIndicator(std::string const& name);
  Indicator::Ptr AddIndicator(std::string const& name);
  void RemoveIndicator(std::string const& name);

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // INDICATOR_OBJECT_FACTORY_H
