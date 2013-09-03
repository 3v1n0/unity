// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "DesktopApplicationManager.h"

namespace unity
{
namespace desktop
{

void Application::LogEvent(ApplicationEventType, ApplicationSubjectPtr const&) const
{}

ApplicationSubject::ApplicationSubject()
  : subject_(zeitgeist_subject_new())
{
  uri.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_uri(subject_));
  });
  uri.SetSetterFunction([this] (std::string const& new_val) {
    if (uri() != new_val)
    {
      zeitgeist_subject_set_uri(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  origin.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_origin(subject_));
  });
  origin.SetSetterFunction([this] (std::string const& new_val) {
    if (origin() != new_val)
    {
      zeitgeist_subject_set_origin(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  text.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_text(subject_));
  });
  text.SetSetterFunction([this] (std::string const& new_val) {
    if (text() != new_val)
    {
      zeitgeist_subject_set_text(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  storage.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_storage(subject_));
  });
  storage.SetSetterFunction([this] (std::string const& new_val) {
    if (storage() != new_val)
    {
      zeitgeist_subject_set_storage(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  current_uri.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_current_uri(subject_));
  });
  current_uri.SetSetterFunction([this] (std::string const& new_val) {
    if (current_uri() != new_val)
    {
      zeitgeist_subject_set_current_uri(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  current_origin.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_current_origin(subject_));
  });
  current_origin.SetSetterFunction([this] (std::string const& new_val) {
    if (current_origin() != new_val)
    {
      zeitgeist_subject_set_current_origin(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  mimetype.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_mimetype(subject_));
  });
  mimetype.SetSetterFunction([this] (std::string const& new_val) {
    if (mimetype() != new_val)
    {
      zeitgeist_subject_set_mimetype(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  interpretation.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_interpretation(subject_));
  });
  interpretation.SetSetterFunction([this] (std::string const& new_val) {
    if (interpretation() != new_val)
    {
      zeitgeist_subject_set_interpretation(subject_, new_val.c_str());
      return true;
    }
    return false;
  });

  manifestation.SetGetterFunction([this] {
    return glib::gchar_to_string(zeitgeist_subject_get_manifestation(subject_));
  });
  manifestation.SetSetterFunction([this] (std::string const& new_val) {
    if (manifestation() != new_val)
    {
      zeitgeist_subject_set_manifestation(subject_, new_val.c_str());
      return true;
    }
    return false;
  });
}

} // namespace desktop
} // namespace unity
