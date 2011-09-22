// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#ifndef UNITY_FADE_CONTROLLER_H_
#define UNITY_FADE_CONTROLLER_H_

#include <Nux/Nux.h>
#include <Nux/TimerProc.h>

#include "Introspectable.h"

namespace unity
{
  
class FadableObject
{
public:
  virtual void SetOpacity(double value) = 0;
  virtual double GetOpacity() = 0;
};

class FadeController
{
public:
  FadeController();
  FadeController(FadableObject* object);
  ~FadeController();

  void FadeIn(unsigned int milliseconds);
  void FadeOut(unsigned int milliseconds);

  void SetOpacity(double opacity);
  double GetOpacity();

  void SetObject(FadableObject* object);

  typedef enum {
    FADING_NONE,
    FADING_IN,
    FADING_OUT
  } FadingStatus;

  FadingStatus GetStatus();

  sigc::signal<void> faded_in;
  sigc::signal<void> faded_out;
  sigc::signal<void, FadingStatus, double> opacity_changed;

private:
  FadableObject* _object;
  FadingStatus _status;
  nux::TimerFunctor* _timer_functor;
  nux::TimerHandle _fade_timehandler;
  sigc::connection _timeout_connection;
  double _start_opacity;

  void TimerTimeOut(void *data);
};

}
#endif
