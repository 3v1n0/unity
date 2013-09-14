// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#ifndef TEST_LOGGER_HELPER_H
#define TEST_LOGGER_HELPER_H

#include <string>
#include <sstream>


namespace unity
{
namespace helper
{

class CaptureLogOutput
{
public:
  CaptureLogOutput();
  ~CaptureLogOutput();
  std::string GetOutput();
private:
  std::ostringstream sout_;
};


void configure_logging(std::string const& env_var = "");

}
}



#endif
