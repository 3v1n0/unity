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

#ifndef UNITY_TIMER_H
#define UNITY_TIMER_H

#include <iosfwd>
#include <string>
#include <glib.h>

namespace unity {
namespace logger {

class Timer
{
public:
    Timer(std::string const& name, std::ostream& out);
    ~Timer();

    void log(std::string const& message);

private:
    std::string name_;
    std::ostream& out_;
    gint64 start_time_;
};

}
}

#endif
