#include <UnityCore/GLibSignal.h>
#include <gtest/gtest.h>

#include "test_glib_signals_utils.h"

using namespace std;
using namespace unity;
using namespace unity::glib;

namespace
{

class TestGLibSignals : public testing::Test
{
public:
  TestGLibSignals()
    : signal0_received_(false)
    , signal1_received_(false)
    , signal2_received_(false)
    , signal3_received_(false)
    , signal4_received_(false)
    , signal5_received_(false)
    , signal6_received_(false)
    , arg1_("")
    , arg2_(-1)
    , arg3_(0.0f)
    , arg4_(0.00)
    , arg5_(false)
    , arg6_(0)
  {
    test_signals_ = static_cast<TestSignals*>(g_object_new(TEST_TYPE_SIGNALS, NULL));
  }

  virtual ~TestGLibSignals()
  {
    if (G_IS_OBJECT(test_signals_))
      g_object_unref(test_signals_);
  }

  void Signal0Callback(TestSignals* signals)
  {
    signal0_received_ = true;
  }

  void Signal1Callback(TestSignals* signals, const char* str)
  {
    arg1_ = str;
    signal1_received_ = true;
  }

  void Signal2Callback(TestSignals* signals, const char* str, int i)
  {
    arg1_ = str;
    arg2_ = i;
    signal2_received_ = true;
  }

  void Signal3Callback(TestSignals* signals, const char* str, int i, float f)
  {
    arg1_ = str;
    arg2_ = i;
    arg3_ = f;
    signal3_received_ = true;
  }

  void Signal4Callback(TestSignals* signals, const char* str, int i, float f, double d) // heh
  {
    arg1_ = str;
    arg2_ = i;
    arg3_ = f;
    arg4_ = d;
    signal4_received_ = true;
  }

  void Signal5Callback(TestSignals* signals, const char* str, int i, float f,
                       double d, gboolean b)
  {
    arg1_ = str;
    arg2_ = i;
    arg3_ = f;
    arg4_ = d;
    arg5_ = b ? true : false;
    signal5_received_ = true;
  }

  gboolean Signal6Callback(TestSignals* signals, const char* str, int i, float f,
                           double d, gboolean b, char c)
  {
    arg1_ = str;
    arg2_ = i;
    arg3_ = f;
    arg4_ = d;
    arg5_ = b ? true : false;
    arg6_ = c;
    signal6_received_ = true;
    return TRUE;
  }

protected:
  TestSignals* test_signals_;

  bool signal0_received_;
  bool signal1_received_;
  bool signal2_received_;
  bool signal3_received_;
  bool signal4_received_;
  bool signal5_received_;
  bool signal6_received_;

  string arg1_;
  int arg2_;
  float arg3_;
  double arg4_;
  bool arg5_;
  char arg6_;
};


TEST_F(TestGLibSignals, TestConstructions)
{
  SignalBase base;

  Signal0<void, TestSignals*> signal0;
  Signal1<void, TestSignals*, string> signal1;
  Signal2<void, TestSignals*, string, int> signal2;
  Signal3<void, TestSignals*, string, int, float> signal3;
  Signal4<void, TestSignals*, string, int, float, double> signal4;
  Signal5<void, TestSignals*, string, int, float, double, gboolean> signal5;
  Signal6<gboolean, TestSignals*, string, int, float, double, gboolean, char> signal6;

  Signal<void, TestSignals*> signal00;
  Signal<void, TestSignals*, string> signal01;
  Signal<void, TestSignals*, string, int> signal02;
  Signal<void, TestSignals*, string, int, float> signal03;
  Signal<void, TestSignals*, string, int, float, double> signal04;
  Signal<void, TestSignals*, string, int, float, double, gboolean> signal05;
  Signal<gboolean, TestSignals*, string, int, float, double, gboolean, char> signal06;
}

TEST_F(TestGLibSignals, TestSignal0)
{
  Signal<void, TestSignals*> signal;
  signal.Connect(test_signals_, "signal0",
                 sigc::mem_fun(this, &TestGLibSignals::Signal0Callback));

  g_signal_emit_by_name(test_signals_, "signal0");

  EXPECT_TRUE(signal0_received_);
  EXPECT_EQ(arg1_, "");
}

TEST_F(TestGLibSignals, TestSignal1)
{
  Signal<void, TestSignals*, const char*> signal;
  signal.Connect(test_signals_, "signal1",
                 sigc::mem_fun(this, &TestGLibSignals::Signal1Callback));

  g_signal_emit_by_name(test_signals_, "signal1", "test");

  EXPECT_TRUE(signal1_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, -1);
}

TEST_F(TestGLibSignals, TestSignal2)
{
  Signal<void, TestSignals*, const char*, int> signal;
  signal.Connect(test_signals_, "signal2",
                 sigc::mem_fun(this, &TestGLibSignals::Signal2Callback));

  g_signal_emit_by_name(test_signals_, "signal2", "test", 100);

  EXPECT_TRUE(signal2_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, 100);
  EXPECT_EQ(arg3_, 0.0f);
}

TEST_F(TestGLibSignals, TestSignal3)
{
  Signal<void, TestSignals*, const char*, int, float> signal;
  signal.Connect(test_signals_, "signal3",
                 sigc::mem_fun(this, &TestGLibSignals::Signal3Callback));

  g_signal_emit_by_name(test_signals_, "signal3", "test", 100, 200.0f);

  EXPECT_TRUE(signal3_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, 100);
  EXPECT_EQ(arg3_, 200.0f);
  EXPECT_EQ(arg4_, 0.00);
}


TEST_F(TestGLibSignals, TestSignal4)
{
  Signal<void, TestSignals*, const char*, int, float, double> signal;
  signal.Connect(test_signals_, "signal4",
                 sigc::mem_fun(this, &TestGLibSignals::Signal4Callback));

  g_signal_emit_by_name(test_signals_, "signal4", "test", 100, 200.0f, 300.00);

  EXPECT_TRUE(signal4_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, 100);
  EXPECT_EQ(arg3_, 200.0f);
  EXPECT_EQ(arg4_, 300.00);
  EXPECT_EQ(arg5_, false);
}

TEST_F(TestGLibSignals, TestSignal5)
{
  Signal<void, TestSignals*, const char*, int, float, double, gboolean> signal;
  signal.Connect(test_signals_, "signal5",
                 sigc::mem_fun(this, &TestGLibSignals::Signal5Callback));

  g_signal_emit_by_name(test_signals_, "signal5", "test", 100, 200.0f, 300.00,
                        TRUE);

  EXPECT_TRUE(signal5_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, 100);
  EXPECT_EQ(arg3_, 200.0f);
  EXPECT_EQ(arg4_, 300.00);
  EXPECT_EQ(arg5_, true);
  EXPECT_EQ(arg6_, 0);
}

// This tests accumulation as well
TEST_F(TestGLibSignals, TestSignal6)
{
  Signal<gboolean, TestSignals*, const char*, int, float, double, gboolean, char> signal;
  signal.Connect(test_signals_, "signal6",
                 sigc::mem_fun(this, &TestGLibSignals::Signal6Callback));

  gboolean ret = FALSE;
  g_signal_emit_by_name(test_signals_, "signal6", "test", 100, 200.0f, 300.00,
                        TRUE, 'x', &ret);

  EXPECT_TRUE(signal6_received_);
  EXPECT_EQ(arg1_, "test");
  EXPECT_EQ(arg2_, 100);
  EXPECT_EQ(arg3_, 200.0f);
  EXPECT_EQ(arg4_, 300.00);
  EXPECT_EQ(arg5_, true);
  EXPECT_EQ(arg6_, 'x');
  EXPECT_EQ(ret, TRUE);
}

TEST_F(TestGLibSignals, TestDisconnection)
{
  Signal<void, TestSignals*> signal;
  signal.Connect(test_signals_, "signal0",
                 sigc::mem_fun(this, &TestGLibSignals::Signal0Callback));
  signal.Disconnect();

  g_signal_emit_by_name(test_signals_, "signal0");

  EXPECT_FALSE(signal0_received_);
}

TEST_F(TestGLibSignals, TestAutoDisconnection)
{
  {
    Signal<void, TestSignals*> signal;
    signal.Connect(test_signals_, "signal0",
                   sigc::mem_fun(this, &TestGLibSignals::Signal0Callback));
  }

  g_signal_emit_by_name(test_signals_, "signal0");

  EXPECT_FALSE(signal0_received_);
}

TEST_F(TestGLibSignals, TestCleanDestruction)
{
  Signal<void, TestSignals*> signal;
  signal.Connect(test_signals_, "signal0",
                 sigc::mem_fun(this, &TestGLibSignals::Signal0Callback));
  g_object_unref(test_signals_);
}

TEST_F(TestGLibSignals, TestManagerConstruction)
{
  SignalManager manager;
}

TEST_F(TestGLibSignals, TestManagerAddition)
{
  SignalManager manager;

  manager.Add(new Signal<void, TestSignals*>(test_signals_,
                                             "signal0",
                                             sigc::mem_fun(this, &TestGLibSignals::Signal0Callback)));
  manager.Add(new Signal<void, TestSignals*, const char*>(test_signals_,
                                                          "signal1",
                                                          sigc::mem_fun(this, &TestGLibSignals::Signal1Callback)));
  manager.Add(new Signal<void, TestSignals*, const char*, int>(test_signals_,
                                                               "signal2",
                                                               sigc::mem_fun(this, &TestGLibSignals::Signal2Callback)));
  manager.Add(new Signal<void, TestSignals*, const char*, int, float>(test_signals_,
                                                                      "signal3",
                                                                      sigc::mem_fun(this, &TestGLibSignals::Signal3Callback)));
  manager.Add(new Signal<void, TestSignals*, const char*, int, float, double>(test_signals_,
                                                                              "signal4",
                                                                              sigc::mem_fun(this, &TestGLibSignals::Signal4Callback)));
  manager.Add(new Signal<void, TestSignals*, const char*, int, float, double, gboolean>(test_signals_,
              "signal5",
              sigc::mem_fun(this, &TestGLibSignals::Signal5Callback)));
  manager.Add(new Signal<gboolean, TestSignals*, const char*, int, float, double, gboolean, char>(test_signals_,
              "signal6",
              sigc::mem_fun(this, &TestGLibSignals::Signal6Callback)));
}

TEST_F(TestGLibSignals, TestManagerConnection)
{
  SignalManager manager;

  manager.Add(new Signal<void, TestSignals*>(test_signals_,
                                             "signal0",
                                             sigc::mem_fun(this, &TestGLibSignals::Signal0Callback)));

  g_signal_emit_by_name(test_signals_, "signal0");
  EXPECT_TRUE(signal0_received_);

  manager.Add(new Signal<void, TestSignals*, const char*>(test_signals_,
                                                          "signal1",
                                                          sigc::mem_fun(this, &TestGLibSignals::Signal1Callback)));
  g_signal_emit_by_name(test_signals_, "signal1", "test");
  EXPECT_TRUE(signal1_received_);

  gboolean ret = FALSE;
  manager.Add(new Signal<gboolean, TestSignals*, const char*, int, float, double, gboolean, char>(test_signals_,
              "signal6",
              sigc::mem_fun(this, &TestGLibSignals::Signal6Callback)));
  g_signal_emit_by_name(test_signals_, "signal6", "test", 100, 1.0f, 100.00, FALSE, 'x', &ret);
  EXPECT_TRUE(signal6_received_);
  EXPECT_TRUE(ret);
}

TEST_F(TestGLibSignals, TestManagerAutoDisconnect)
{
  {
    SignalManager manager;
    manager.Add(new Signal<void, TestSignals*>(test_signals_,
                                               "signal0",
                                               sigc::mem_fun(this, &TestGLibSignals::Signal0Callback)));
  }

  g_signal_emit_by_name(test_signals_, "signal0");
  EXPECT_FALSE(signal0_received_);
}

TEST_F(TestGLibSignals, TestManagerDisconnection)
{
  SignalManager manager;

  manager.Add(new Signal<void, TestSignals*>(test_signals_,
                                             "signal0",
                                             sigc::mem_fun(this, &TestGLibSignals::Signal0Callback)));
  manager.Disconnect(test_signals_, "signal0");

  g_signal_emit_by_name(test_signals_, "signal0");
  EXPECT_FALSE(signal0_received_);
}

}
