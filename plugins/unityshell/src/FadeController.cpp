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

#include "FadeController.h"

namespace unity
{

FadeController::FadeController()
{
  _object = NULL;
  _status = FADING_NONE;

  _timer_functor = new nux::TimerFunctor();
  _timeout_connection = _timer_functor->OnTimerExpired.connect(
                        sigc::mem_fun(this, &FadeController::TimerTimeOut));
}

FadeController::FadeController(FadableObject *object)
{
  _object = object;
  _status = FADING_NONE;

  _timer_functor = new nux::TimerFunctor();
  _timeout_connection = _timer_functor->OnTimerExpired.connect(
                        sigc::mem_fun(this, &FadeController::TimerTimeOut));
}

FadeController::~FadeController()
{
  if (_fade_timehandler.IsValid())
    nux::GetTimer().RemoveTimerHandler(_fade_timehandler);

  _timeout_connection.disconnect();

  delete _timer_functor;
}

void
FadeController::SetObject(FadableObject* object)
{
  if (_fade_timehandler.IsValid())
    nux::GetTimer().RemoveTimerHandler(_fade_timehandler);

  _object = object;
}

void
FadeController::FadeIn(unsigned int ms)
{
  if (!_object)
    return;

  if (_fade_timehandler.IsValid())
  {
    if (_status == FADING_IN && _fade_timehandler.GetElapsedTimed() > 1)
      return;

    nux::GetTimer().RemoveTimerHandler(_fade_timehandler);
  }

  if (_object->GetOpacity() >= 1.0)
  {
    _status = FADING_NONE;
    return;
  }

  _status = FADING_IN;
  _start_opacity = _object->GetOpacity();
  _fade_timehandler = nux::GetTimer().AddPeriodicTimerHandler(1, ms, _timer_functor, NULL);
}

void
FadeController::FadeOut(unsigned int ms)
{
  if (!_object)
    return;

  if (_fade_timehandler.IsValid())
  {
    if (_status == FADING_OUT && _fade_timehandler.GetElapsedTimed() > 1)
      return;

    nux::GetTimer().RemoveTimerHandler(_fade_timehandler);
  }

  if (_object->GetOpacity() <= 0.0)
  {
    _status = FADING_NONE;
    return;
  }

  _status = FADING_OUT;
  _start_opacity = _object->GetOpacity();
  _fade_timehandler = nux::GetTimer().AddPeriodicTimerHandler(1, ms, _timer_functor, NULL);
}

void
FadeController::TimerTimeOut(void *data)
{
  double progress = _fade_timehandler.GetProgress();
  double opacity = _start_opacity;

  if (_status == FADING_IN)
    opacity += progress;
  else if (_status == FADING_OUT)
    opacity -= progress;

  _object->SetOpacity(opacity);
  opacity_changed.emit(_status, opacity);

  if (progress >= 1.0f ||
      (_object->GetOpacity() <= 0.0f && _status == FADING_OUT) ||
      (_object->GetOpacity() >= 1.0f && _status == FADING_IN) ||
      _status == FADING_NONE)
  {
    nux::GetTimer().RemoveTimerHandler(_fade_timehandler);

    if (_status == FADING_IN)
      faded_in.emit();
    else if (_status == FADING_OUT)
      faded_out.emit();

    _status = FADING_NONE;
  }
}

void
FadeController::SetOpacity(double opacity)
{
  if (_object)
  {
    _object->SetOpacity(opacity);
    opacity_changed.emit(FADING_NONE, opacity);
  }
}

double
FadeController::GetOpacity()
{
  if (_object)
    return _object->GetOpacity();

  return 0.0;
}

} //namespace
