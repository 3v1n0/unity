/*
 * Copyright 2011 Canonical Ltd.
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/Button.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/CheckBox.h"
#include "Nux/SpinBox.h"
#include "Nux/EditTextBox.h"
#include "Nux/StaticText.h"
#include "Nux/RangeValueInteger.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>
#include <gtk/gtk.h>

#include "SwitcherController.h"
#include "SwitcherView.h"
#include "MockLauncherIcon.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/UnitySettings.h"

using namespace unity::switcher;
using namespace unity::ui;
using unity::launcher::AbstractLauncherIcon;
using unity::launcher::MockLauncherIcon;

static bool enable_flipping = false;

static Controller::Ptr controller;

static gboolean on_timeout(gpointer data)
{
  if (!enable_flipping)
    return TRUE;

  Controller* self = static_cast<Controller*>(data);
  self->Next();

  return TRUE;
}

void OnFlippingChanged (bool value)
{
  enable_flipping = value;
}

void OnBorderSizeChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->border_size = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnFlatSpacingSizeChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->flat_spacing = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnTextSizeChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->text_size = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnIconSizeChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->icon_size = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnTileSizeChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->tile_size = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnAnimationLengthChanged (nux::RangeValueInteger *self)
{
  controller->GetView ()->animation_length = self->GetValue ();
  controller->GetView ()->QueueDraw ();
}

void OnNumIconsChanged (nux::SpinBox *self)
{
  controller->Hide();

  std::vector<nux::ObjectPtr<AbstractLauncherIcon>> icons;
  for (int i = 0; i < self->GetValue (); i++)
    icons.push_back(nux::ObjectPtr<AbstractLauncherIcon>(new MockLauncherIcon()));

  controller->Show(ShowMode::ALL, SortMode::FOCUS_ORDER, icons);
}

void OnNextClicked (nux::View *sender)
{
  controller->Next ();
}

void OnDetailClicked (nux::View *sender)
{
  controller->NextDetail ();
}

void OnPreviousClicked (nux::View *sender)
{
  controller->Prev();
}

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  nux::VLayout* layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

  controller = std::make_shared<Controller>();

  layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  layout->SetHorizontalExternalMargin (10);
  nux::GetWindowThread()->SetLayout(layout);

  std::vector<nux::ObjectPtr<AbstractLauncherIcon>> icons;
  for (int i = 0; i < 9; i++)
    icons.push_back(nux::ObjectPtr<AbstractLauncherIcon>(new MockLauncherIcon()));

  controller->Show(ShowMode::ALL, SortMode::FOCUS_ORDER, icons);

  controller->GetView ()->render_boxes = true;

  nux::CheckBox* flipping_check = new nux::CheckBox(TEXT("Enable Automatic Flipping"), false, NUX_TRACKER_LOCATION);
  flipping_check->SetMaximumWidth(250);
  flipping_check->state_change.connect (sigc::ptr_fun (OnFlippingChanged));
  layout->AddView(flipping_check, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  nux::HLayout* num_icons_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  num_icons_layout->SetMaximumWidth(250);
  num_icons_layout->SetMaximumHeight(30);
  num_icons_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* num_icons_label = new nux::StaticText(TEXT("Num Icons:"), NUX_TRACKER_LOCATION);
  num_icons_layout->AddView(num_icons_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::SpinBox * num_icons_spin = new nux::SpinBox (icons.size (), 1, 2, 100, NUX_TRACKER_LOCATION);
  num_icons_spin->sigValueChanged.connect (sigc::ptr_fun (OnNumIconsChanged));
  num_icons_layout->AddView(num_icons_spin, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(num_icons_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);



  nux::HLayout* border_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  border_layout->SetMaximumWidth(250);
  border_layout->SetMaximumHeight(30);
  border_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* border_label = new nux::StaticText(TEXT("Border Size:"), NUX_TRACKER_LOCATION);
  border_layout->AddView(border_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * border_size_range = new nux::RangeValueInteger (controller->GetView ()->border_size, 0, 200, NUX_TRACKER_LOCATION);
  border_size_range->sigValueChanged.connect (sigc::ptr_fun (OnBorderSizeChanged));
  border_layout->AddView(border_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(border_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);



  nux::HLayout* flat_spacing_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  flat_spacing_layout->SetMaximumWidth(250);
  flat_spacing_layout->SetMaximumHeight(30);
  flat_spacing_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* flat_spacing_label = new nux::StaticText(TEXT("Flat Spacing:"), NUX_TRACKER_LOCATION);
  flat_spacing_layout->AddView(flat_spacing_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * flat_spacing_size_range = new nux::RangeValueInteger (controller->GetView ()->flat_spacing, 0, 200, NUX_TRACKER_LOCATION);
  flat_spacing_size_range->sigValueChanged.connect (sigc::ptr_fun (OnFlatSpacingSizeChanged));
  flat_spacing_layout->AddView(flat_spacing_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(flat_spacing_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);
  

  nux::HLayout* text_size_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  text_size_layout->SetMaximumWidth(250);
  text_size_layout->SetMaximumHeight(30);
  text_size_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* text_size_label = new nux::StaticText(TEXT("Text Size:"), NUX_TRACKER_LOCATION);
  text_size_layout->AddView(text_size_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * text_size_size_range = new nux::RangeValueInteger (controller->GetView ()->text_size, 0, 200, NUX_TRACKER_LOCATION);
  text_size_size_range->sigValueChanged.connect (sigc::ptr_fun (OnTextSizeChanged));
  text_size_layout->AddView(text_size_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(text_size_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  nux::HLayout* icon_size_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  icon_size_layout->SetMaximumWidth(250);
  icon_size_layout->SetMaximumHeight(30);
  icon_size_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* icon_size_label = new nux::StaticText(TEXT("Icon Size:"), NUX_TRACKER_LOCATION);
  icon_size_layout->AddView(icon_size_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * icon_size_size_range = new nux::RangeValueInteger (controller->GetView ()->icon_size, 0, 200, NUX_TRACKER_LOCATION);
  icon_size_size_range->sigValueChanged.connect (sigc::ptr_fun (OnIconSizeChanged));
  icon_size_layout->AddView(icon_size_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(icon_size_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  nux::HLayout* tile_size_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  tile_size_layout->SetMaximumWidth(250);
  tile_size_layout->SetMaximumHeight(30);
  tile_size_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* tile_size_label = new nux::StaticText(TEXT("Tile Size:"), NUX_TRACKER_LOCATION);
  tile_size_layout->AddView(tile_size_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * tile_size_size_range = new nux::RangeValueInteger (controller->GetView ()->tile_size, 0, 200, NUX_TRACKER_LOCATION);
  tile_size_size_range->sigValueChanged.connect (sigc::ptr_fun (OnTileSizeChanged));
  tile_size_layout->AddView(tile_size_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(tile_size_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  nux::HLayout* animation_length_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  animation_length_layout->SetMaximumWidth(250);
  animation_length_layout->SetMaximumHeight(30);
  animation_length_layout->SetHorizontalInternalMargin (10);

  nux::StaticText* animation_length_label = new nux::StaticText(TEXT("Animation Length:"), NUX_TRACKER_LOCATION);
  animation_length_layout->AddView(animation_length_label, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::RangeValueInteger * animation_length_size_range = new nux::RangeValueInteger (controller->GetView ()->animation_length, 0, 2000, NUX_TRACKER_LOCATION);
  animation_length_size_range->sigValueChanged.connect (sigc::ptr_fun (OnAnimationLengthChanged));
  animation_length_layout->AddView(animation_length_size_range, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FIX);

  layout->AddView(animation_length_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  nux::HLayout* control_buttons_layout = new nux::HLayout("", NUX_TRACKER_LOCATION);
  control_buttons_layout->SetMaximumWidth(250);
  control_buttons_layout->SetMaximumHeight(30);
  control_buttons_layout->SetHorizontalInternalMargin (10);

  nux::Button* prev_button = new nux::Button ("Previous", NUX_TRACKER_LOCATION);
  prev_button->state_change.connect (sigc::ptr_fun (OnPreviousClicked));
  control_buttons_layout->AddView(prev_button, 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  nux::Button* next_button = new nux::Button ("Next", NUX_TRACKER_LOCATION);
  next_button->state_change.connect (sigc::ptr_fun (OnNextClicked));
  control_buttons_layout->AddView(next_button, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);

  nux::Button* detail_button = new nux::Button ("Detail", NUX_TRACKER_LOCATION);
  detail_button->state_change.connect (sigc::ptr_fun (OnDetailClicked));
  control_buttons_layout->AddView(detail_button, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);

  layout->AddView(control_buttons_layout, 1, nux::MINOR_POSITION_END, nux::MINOR_SIZE_FULL);


  layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);

  nux::BaseTexture *background = nux::CreateTexture2DFromFile("/usr/share/backgrounds/warty-final-ubuntu.png", -1, true);
  nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_ = background->GetDeviceTexture();


  g_timeout_add(1500, on_timeout, controller.get());
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);
  nux::NuxInitialize(0);

  unity::Settings settings;
  BackgroundEffectHelper::blur_type = unity::BLUR_ACTIVE;
  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Switcher"), 1200, 600, 0, &ThreadWidgetInit, 0);
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller(tick_source);

  wt->Run(NULL);
  delete wt;
  return 0;
}
