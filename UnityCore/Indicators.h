// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
  typedef std::list<Indicator::Ptr> IndicatorsList;

  Indicators();
  virtual ~Indicators();

  IndicatorsList GetIndicators() const;

  /**
   * internal
   */
  void ActivateEntry(std::string const& entry_id, nux::Rect const& geometry);

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
  virtual void OnEntryShowMenu(std::string const& entry_id, unsigned int xid,
                               int x, int y, unsigned int button,
                               unsigned int timestamp) = 0;

  /**
   * internal
   */
  virtual void OnEntrySecondaryActivate(std::string const& entry_id,
                                        unsigned int timestamp) = 0;

  /**
   * internal
   */
  virtual void OnShowAppMenu(unsigned int xid, int x, int y,
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
  sigc::signal<void, std::string const&, nux::Rect const&> on_entry_activated;

  /**
   * internal
   */
  sigc::signal<void> on_synced;

  /**
   * The service is about to show a menu.
   * @param entry_id entry id
   * @param xid window xid
   * @param x coordinate
   * @param y coordinate
   * @param button pressed button
   * @param timestamp current time
   */
  sigc::signal<void, std::string const&, unsigned int, int, int, unsigned int, unsigned int> on_entry_show_menu;

  /**
   * The service is about to show an appmenu.
   * @param xid window xid
   * @param x coordinate
   * @param y coordinate
   * @param timestamp current time
   */
  sigc::signal<void, unsigned int, int, int, unsigned int> on_show_appmenu;

protected:
  Indicator::Ptr GetIndicator(std::string const& name);
  Indicator::Ptr AddIndicator(std::string const& name);
  void RemoveIndicator(std::string const& name);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif // INDICATOR_OBJECT_FACTORY_H
