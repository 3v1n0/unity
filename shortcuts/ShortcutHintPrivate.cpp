/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "ShortcutHintPrivate.h"
#include "UnityCore/GLibWrapper.h"

#include <boost/algorithm/string/replace.hpp>

namespace unity
{
namespace shortcut
{
namespace impl
{

std::string GetMetaKey(std::string const& scut)
{
  size_t index = scut.find_last_of( ">");

  return std::string(scut.begin(), scut.begin() + index + 1);
}

std::string FixShortcutFormat(std::string const& scut)
{
  std::string ret(scut.begin(), scut.end() - 1);
  
  boost::replace_all(ret, "<", "");
  boost::replace_all(ret, ">", " + ");
  
  if (scut[scut.size()-1] != '>')
    ret += scut[scut.size()-1];
    
  return ret;
}

std::string GetTranslatableLabel(std::string const& scut)
{
  guint accelerator_key;
  GdkModifierType accelerator_mods;

  gtk_accelerator_parse(scut.c_str(),
                        &accelerator_key,
                        &accelerator_mods);

  std::string temp(glib::String(gtk_accelerator_get_label(accelerator_key, accelerator_mods)).Str());

  // gtk_accelerator_get_label adds an extra '+' at the end of the label.
  if (temp.length() > 0)
  {
    std::string::iterator it = temp.end() - 1;
    if (*it == '+')
      temp.erase(it);
  }

  // Adds an extra space around the '+'.
  boost::replace_all(temp, "+", " + ");
    
  return temp;
}

std::string FixMouseShortcut(std::string const& scut)
{
  std::string ret(scut);
  
  boost::replace_all(ret, "Button1", _("Left Mouse"));
  boost::replace_all(ret, "Button2", _("Middle Mouse"));
  boost::replace_all(ret, "Button3", _("Right Mouse"));
      
  return ret;
}

std::string ProperCase(std::string const& str)
{
  std::string ret = str;
  
	bool cap_next = true;

	for (unsigned int i = 0; i < ret.length(); ++i)
  {
    if (cap_next and isalpha(ret[i]))
    {
      ret[i]=toupper(ret[i]);
      cap_next = false;
    }
    else
    {
      cap_next = ispunct(ret[i]) || isspace(ret[i]);
    }
  }
  
  return ret;
}

} // namespace impl
} // namespace shortcut
} // namespace unity

