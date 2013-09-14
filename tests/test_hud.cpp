#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Hud.h>
#include <sigc++/connection.h>
#include "test_utils.h"

using namespace std;

namespace unity
{
namespace hud
{


namespace
{

static void WaitForConnect(Hud::Ptr hud)
{
  ::Utils::WaitUntil([hud]() { return hud->connected(); }, true, 10, "Timed out waiting for hud connection");
}

}

TEST(TestHud, TestConstruction)
{
  Hud::Ptr hud(new Hud("com.canonical.Unity.Test", "/com/canonical/hud"));
  WaitForConnect(hud);
}

TEST(TestHud, TestQueryReturn)
{
  Hud::Ptr hud(new Hud("com.canonical.Unity.Test", "/com/canonical/hud"));
  WaitForConnect(hud);

  bool query_return_result = false;
  Hud::Queries queries;
  
  // make sure we receive the queries
  auto query_connection = [&query_return_result, &queries](Hud::Queries const& queries_) 
  { 
    query_return_result = true;
    queries = queries_;
  };
   
  sigc::connection connection = hud->queries_updated.connect(query_connection);
 
  // next check we get 30 entries from this specific known callback
  hud->RequestQuery("Request30Queries");

  ::Utils::WaitUntil([&queries, &query_return_result]() { return query_return_result && queries.size() > 0; },
                     true, 10, "Timed out waiting for hud queries");

  // finally close the connection - Nothing to check for here
  hud->CloseQuery();
  connection.disconnect();
}

TEST(TestHud, TestSigEmission)
{
  Hud::Ptr hud(new Hud("com.canonical.Unity.Test", "/com/canonical/hud"));
  WaitForConnect(hud);

  // checks that the signal emission from Hud is working correctly
  // the service is setup to emit the same signal every 1000ms
  // using the same query key as its StarQuery method
  // so calling StartQuery and listening we expect > 1 
  // signal emission
  int number_signals_found = 0;

  // make sure we receive the queries
  auto query_connection = [&number_signals_found](Hud::Queries const&) 
  { 
    number_signals_found += 1;
  };
   
  sigc::connection connection = hud->queries_updated.connect(query_connection);
 
  hud->RequestQuery("Request30Queries");

  ::Utils::WaitUntil([&number_signals_found]() { return number_signals_found > 1; },
                     true, 10, "Timed out waiting for hud signals");

  // finally close the connection - Nothing to check for here
  hud->CloseQuery();
  connection.disconnect();
}

}
}
