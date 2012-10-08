#include <gtest/gtest.h>

#include <UnityCore/DBusIndicators.h>

#include "test_utils.h"

using namespace unity::indicator;

namespace
{

class DBusIndicatorsTest : public DBusIndicators
{
public:
  DBusIndicatorsTest() : DBusIndicators("com.canonical.Unity.Test")
  {
  }

  using DBusIndicators::IsConnected;
};

class TestDBusIndicators : public ::testing::Test
{
public:
  TestDBusIndicators()
  {
  }
  GMainLoop* loop_;
  DBusIndicatorsTest* dbus_indicators;
  int nChecks;
  GDBusConnection* session;
};

TEST_F(TestDBusIndicators, TestConstruction)
{
  loop_ = g_main_loop_new(NULL, FALSE);
  dbus_indicators = new DBusIndicatorsTest ();

  // wait until the dbus indicator has connected to the panel service
  auto timeout_check = [] (gpointer data) -> gboolean
  {
    TestDBusIndicators* self = static_cast<TestDBusIndicators*>(data);
    self->nChecks++;
    bool quit_loop = self->dbus_indicators->IsConnected() || self->nChecks > 99;
    if (quit_loop)
      g_main_loop_quit(self->loop_);
    return !quit_loop;
  };

  nChecks = 0;
  g_timeout_add(100, timeout_check, this);

  g_main_loop_run(loop_);

  EXPECT_EQ(dbus_indicators->IsConnected(), true);
}

TEST_F(TestDBusIndicators, TestSync)
{
  loop_ = g_main_loop_new(NULL, FALSE);
  dbus_indicators = new DBusIndicatorsTest ();

  // wait until the dbus indicator gets any indicator from the panel service
  auto timeout_check = [] (gpointer data) -> gboolean
  {
    TestDBusIndicators* self = static_cast<TestDBusIndicators*>(data);
    self->nChecks++;
    bool quit_loop = !self->dbus_indicators->GetIndicators().empty() || self->nChecks > 99;
    if (quit_loop)
      g_main_loop_quit(self->loop_);
    return !quit_loop;
  };

  nChecks = 0;
  g_timeout_add(100, timeout_check, this);

  g_main_loop_run(loop_);

  EXPECT_EQ(dbus_indicators->GetIndicators().size(), 1);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().size(), 2);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().front()->id(), "test_entry_id");
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().back()->id(), "test_entry_id2");

  // Tell the service to trigger a resync and to send the entries in the reverse order
  session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
  g_dbus_connection_set_exit_on_close(session, FALSE);

  g_dbus_connection_call_sync(session,
                              "com.canonical.Unity.Test",
                              "/com/canonical/Unity/Panel/Service",
                              "com.canonical.Unity.Panel.Service",
                              "TriggerResync1",
                              NULL, /* params */
                              NULL, /* ret type */
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              NULL,
                              NULL);
  // wait for the Resync to come and go
  auto timeout_check_2 = [] (gpointer data) -> gboolean
  {
    TestDBusIndicators* self = static_cast<TestDBusIndicators*>(data);

    GVariant *ret = g_dbus_connection_call_sync(self->session,
                              "com.canonical.Unity.Test",
                              "/com/canonical/Unity/Panel/Service",
                              "com.canonical.Unity.Panel.Service",
                              "TriggerResync1Sent",
                              NULL, /* params */
                              G_VARIANT_TYPE("(b)"), /* ret type */
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              NULL,
                              NULL);
    self->nChecks++;
    bool quit_loop = g_variant_get_boolean(g_variant_get_child_value(ret, 0)) || self->nChecks > 99;
    if (quit_loop)
      g_main_loop_quit(self->loop_);
    return !quit_loop;
  };

  nChecks = 0;
  g_timeout_add(100, timeout_check_2, this);

  g_main_loop_run(loop_);

  EXPECT_EQ(dbus_indicators->GetIndicators().size(), 1);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().size(), 2);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().front()->id(), "test_entry_id2");
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().back()->id(), "test_entry_id");
}

}
