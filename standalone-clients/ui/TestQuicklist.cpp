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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <glib.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowThread.h"

#include "QuicklistView.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "nux_test_framework.h"
#include "nux_automated_test_framework.h"

#define WIN_WIDTH  400
#define WIN_HEIGHT 400

class TestQuicklist: public NuxTestFramework
{
public:
  TestQuicklist(const char *program_name, int window_width, int window_height, int program_life_span);
  ~TestQuicklist();

  virtual void UserInterfaceSetup();
  int ItemNaturalPosition(QuicklistMenuItem* item);
  bool HasNthItemActivated(unsigned int index);

  QuicklistView* quicklist_;
  std::map<QuicklistMenuItem*,bool> activated_;

private:
  QuicklistMenuItemSeparator* createSeparatorItem();
  QuicklistMenuItemLabel* createLabelItem(std::string const& label, bool enabled = true);
  QuicklistMenuItemCheckmark* createCheckmarkItem(std::string const& label, bool enabled, bool checked);
  QuicklistMenuItemRadio* createRadioItem(std::string const& label, bool enabled, bool checked);
  void AddItem(QuicklistMenuItem* item);

  void connectToActivatedSignal(DbusmenuMenuitem* item);
  static void activatedCallback(DbusmenuMenuitem* item, int time, gpointer data);

  std::map<DbusmenuMenuitem*, QuicklistMenuItem*> menus2qitem_;
};

TestQuicklist::TestQuicklist(const char *program_name, int window_width, int window_height, int program_life_span)
  : NuxTestFramework(program_name, window_width, window_height, program_life_span),
  quicklist_(nullptr)
{}

TestQuicklist::~TestQuicklist()
{
  if (quicklist_)
    quicklist_->UnReference();
}

int TestQuicklist::ItemNaturalPosition(QuicklistMenuItem* item)
{
  int pos = 1;

  for (auto it : quicklist_->GetChildren())
  {
    if (it == item)
      return pos;

    if (it->GetItemType() != MENUITEM_TYPE_SEPARATOR)
      pos++;
  }

  return -1;
}

bool TestQuicklist::HasNthItemActivated(unsigned int index)
{
  return activated_[quicklist_->GetNthItems(index)];
}

void TestQuicklist::activatedCallback(DbusmenuMenuitem* item, int time, gpointer data)
{
  auto self = static_cast<TestQuicklist*>(data);
  QuicklistMenuItem* qitem = self->menus2qitem_[item];

  if (!self->activated_[qitem])
  {
    self->activated_[qitem] = true;
    g_debug("Quicklist-item %d activated", self->ItemNaturalPosition(qitem));
  }
}

void TestQuicklist::connectToActivatedSignal(DbusmenuMenuitem* item)
{
  g_signal_connect (item,
                    DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (&TestQuicklist::activatedCallback),
                    this);
}

QuicklistMenuItemSeparator* TestQuicklist::createSeparatorItem()
{
  DbusmenuMenuitem*           item      = NULL;
  QuicklistMenuItemSeparator* separator = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);

  separator = new QuicklistMenuItemSeparator (item, true);
  menus2qitem_[item] = separator;

  return separator;
}

QuicklistMenuItemRadio* TestQuicklist::createRadioItem(std::string const& label, bool enabled, bool checked)
{
  DbusmenuMenuitem*       item  = NULL;
  QuicklistMenuItemRadio* radio = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  label.c_str());

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_RADIO);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      (checked ?
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED :
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED
                                      ));

  connectToActivatedSignal(item);
  radio = new QuicklistMenuItemRadio (item, true);
  menus2qitem_[item] = radio;

  return radio;
}

QuicklistMenuItemCheckmark* TestQuicklist::createCheckmarkItem(std::string const& label, bool enabled, bool checked)
{
  DbusmenuMenuitem*           item      = NULL;
  QuicklistMenuItemCheckmark* checkmark = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  label.c_str());

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                  DBUSMENU_MENUITEM_TOGGLE_CHECK);

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  dbusmenu_menuitem_property_set_int (item,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      (checked ?
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED :
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED
                                      ));

  connectToActivatedSignal(item);

  checkmark = new QuicklistMenuItemCheckmark (item, true);
  menus2qitem_[item] = checkmark;
    
  return checkmark;
}

QuicklistMenuItemLabel* TestQuicklist::createLabelItem(std::string const& title, bool enabled)
{
  DbusmenuMenuitem*       item  = NULL;
  QuicklistMenuItemLabel* label = NULL;

  item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  title.c_str());

  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       enabled);

  connectToActivatedSignal(item);

  label = new QuicklistMenuItemLabel (item, true);
  menus2qitem_[item] = label;

  return label;
}

void TestQuicklist::AddItem(QuicklistMenuItem* item)
{
  if (!quicklist_)
    return;

  quicklist_->AddMenuItem(item);
}

void TestQuicklist::UserInterfaceSetup()
{
  QuicklistMenuItem *item;

  quicklist_ = new QuicklistView();
  quicklist_->EnableQuicklistForTesting(true);
  quicklist_->SetBaseXY(0, 0);

  item = createLabelItem("Item1, normal");
  AddItem(item);

  item = createSeparatorItem();
  AddItem(item);

  item = createRadioItem("Item2, radio, checked", true, true);
  AddItem(item);

  item = createRadioItem("Item3, radio, unchecked", true, false);
  AddItem(item);

  item = createRadioItem("Item4, disabled radio, checked", false, true);
  AddItem(item);

  item = createRadioItem("Item5, disabled radio, unchecked", false, false);
  AddItem(item);

  item = createCheckmarkItem("Item6, checkmark, checked", true, true);
  AddItem(item);

  item = createCheckmarkItem("Item7, checkmark, unchecked", true, false);
  AddItem(item);

  item = createCheckmarkItem("Item8, disabled checkmark, checked", false, true);
  AddItem(item);

  item = createCheckmarkItem("Item9, disabled checkmark, unchecked", false, false);
  AddItem(item);

  item = createLabelItem("Item10, disabled", false);
  AddItem(item);

  quicklist_->ShowWindow(true);

  auto wt = static_cast<nux::WindowThread*>(window_thread_);
  nux::ColorLayer background (nux::Color (0x772953));
  wt->SetWindowBackgroundPaintLayer(&background);
}

TestQuicklist *test_quicklist = NULL;

void TestingThread(nux::NThread *thread, void *user_data)
{
  while (test_quicklist->ReadyToGo() == false)
  {
    nuxDebugMsg("Waiting to start");
    nux::SleepForMilliseconds(300);
  }

  nux::SleepForMilliseconds(1300);

  auto *wnd_thread = static_cast<nux::WindowThread*>(user_data);

  NuxAutomatedTestFramework test(wnd_thread);

  test.Startup();

  for (auto child : test_quicklist->quicklist_->GetChildren())
  {
    test.ViewSendMouseMotionToCenter(child);
    test.ViewSendMouseClick(child, 1);
    bool activated = test_quicklist->activated_[child];
    bool should_be_activated = (child->GetItemType() != MENUITEM_TYPE_SEPARATOR && child->GetEnabled());

    std::string msg = std::string(child->GetLabel());
    msg += should_be_activated ? " | Activated" : " | NOT Activated";

    test.TestReportMsg(activated == should_be_activated, msg.c_str());
    nux::SleepForMilliseconds(200);
  }

  if (test.WhenDoneTerminateProgram())
  {
    wnd_thread->ExitMainLoop();
  }
  nuxDebugMsg("Exit testing thread");
}

int
main (int argc, char **argv)
{
  gtk_init(&argc, &argv);
  nuxAssertMsg(XInitThreads() > 0, "XInitThreads has failed");

  test_quicklist = new TestQuicklist("Quicklist Test", WIN_WIDTH, WIN_HEIGHT, 100000);
  test_quicklist->Startup();
  test_quicklist->UserInterfaceSetup();

  auto *test_thread = nux::CreateSystemThread(NULL, &TestingThread, test_quicklist->GetWindowThread());
  test_thread->Start(test_quicklist);

  test_quicklist->Run();

  g_assert_cmpint (test_quicklist->HasNthItemActivated(0), ==, true);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(1), ==, false);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(2), ==, true);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(3), ==, true);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(4), ==, false);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(5), ==, false);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(6), ==, true);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(7), ==, true);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(8), ==, false);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(9), ==, false);
  g_assert_cmpint (test_quicklist->HasNthItemActivated(10), ==, false);

  delete test_thread;
  delete test_quicklist;

  return 0;
}
