/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/TextEntry.h"

#ifndef NUXTESTFRAMEWORK_H
#define NUXTESTFRAMEWORK_H

class NuxTestFramework
{
public:
  NuxTestFramework(const char* program_name, int window_width, int window_height, int program_life_span);
  virtual ~NuxTestFramework();

  virtual void Startup();
  virtual void UserInterfaceSetup();
  virtual void Run();

  bool ReadyToGo();

  nux::WindowThread* GetWindowThread();

public:
  std::string program_name_;
  int program_life_span_;                  //!< The program will auto-terminate after a delay in milliseconds.
  nux::TimeOutSignal *timeout_signal_;

  nux::WindowThread *window_thread_;

  int window_width_;
  int window_height_;

private:
  void ProgramExitCall(void *data);
  void WaitForConfigureEvent(int x, int y, int width, int height);
  bool ready_to_go_;
};

#endif // NUXTESTFRAMEWORK_H

