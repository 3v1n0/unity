#include <gtest/gtest.h>

#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DBusIndicators.h>

#include "test_utils.h"

using namespace unity;
using namespace unity::indicator;

namespace
{

class DBusIndicatorsTest : public DBusIndicators
{
public:
  DBusIndicatorsTest() : DBusIndicators("com.canonical.Unity.Test")
  {
  }

  bool HasIndicators() const
  {
    return !GetIndicators().empty();
  }

  using DBusIndicators::IsConnected;
};

class TestDBusIndicators : public ::testing::Test
{
public:
  void SetUp()
  {
    session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    g_dbus_connection_set_exit_on_close(session, FALSE);

    dbus_indicators.reset(new DBusIndicatorsTest ());

    // wait until the dbus indicator has connected to the panel service
    Utils::WaitUntil(sigc::mem_fun(*dbus_indicators, &DBusIndicatorsTest::IsConnected), true, 5);
  }

  bool TriggerResync1Sent() const
  {
    GVariant *ret = CallPanelMethod("TriggerResync1Sent");
    return g_variant_get_boolean(g_variant_get_child_value(ret, 0));
  }

  GVariant* CallPanelMethod(std::string const& name) const
  {
    return g_dbus_connection_call_sync(session,
                                       "com.canonical.Unity.Test",
                                       "/com/canonical/Unity/Panel/Service",
                                       "com.canonical.Unity.Panel.Service",
                                       name.c_str(),
                                       NULL,
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       NULL);
  }

  glib::Object<GDBusConnection> session;
  std::shared_ptr<DBusIndicatorsTest> dbus_indicators;
};

TEST_F(TestDBusIndicators, TestConstruction)
{
  EXPECT_EQ(dbus_indicators->IsConnected(), true);
}

TEST_F(TestDBusIndicators, TestSync)
{
  // wait until the dbus indicator gets any indicator from the panel service
  Utils::WaitUntil(sigc::mem_fun(*dbus_indicators, &DBusIndicatorsTest::HasIndicators), true, 5);

  EXPECT_EQ(dbus_indicators->GetIndicators().size(), 1);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().size(), 2);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().front()->id(), "test_entry_id");
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().back()->id(), "test_entry_id2");

  // Tell the service to trigger a resync and to send the entries in the reverse order
  CallPanelMethod("TriggerResync1");

  Utils::WaitUntil(sigc::mem_fun(this, &TestDBusIndicators::TriggerResync1Sent), true, 5);
  // We know the resync has been sent, but it may have not been processed
  // so do one interation of the main loop more
  g_main_context_iteration(NULL, TRUE);

  EXPECT_EQ(dbus_indicators->GetIndicators().size(), 1);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().size(), 2);
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().front()->id(), "test_entry_id2");
  EXPECT_EQ(dbus_indicators->GetIndicators().front()->GetEntries().back()->id(), "test_entry_id");
}

}
