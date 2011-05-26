#include "Timer.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <gtest/gtest.h>

using namespace std;

namespace {

pair<float, string> get_elapsed_time_and_message(string const& line)
{
  stringstream sin(line);
  pair<float, string> result;
  sin >> result.first;
  getline(sin, result.second);
  return result;
}

TEST(TestBlockTimer, TestOutput) {
  stringstream sout;
  {
    unity::logger::BlockTimer t("My timer", sout);
  }
  string output = sout.str();
  boost::trim(output);
  vector<string> lines;
  boost::split(lines, output, boost::is_any_of("\n"));
  EXPECT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[0], "STARTED (My timer)");
  pair<float, string> finished = get_elapsed_time_and_message(lines[1]);
  EXPECT_GE(finished.first, 0);
  EXPECT_EQ(finished.second, "s: FINISHED (My timer)");
}

TEST(TestBlockTimer, TestMultipleWithMessage) {
  stringstream sout;
  {
    unity::logger::BlockTimer first("first", sout);
    unity::logger::BlockTimer second("second", sout);
    first.log("message");
  }
  string output = sout.str();
  boost::trim(output);
  vector<string> lines;
  boost::split(lines, output, boost::is_any_of("\n"));
  EXPECT_EQ(lines.size(), 5);
  EXPECT_EQ(lines[0], "STARTED (first)");
  EXPECT_EQ(lines[1], "STARTED (second)");
  pair<float, string> msg = get_elapsed_time_and_message(lines[2]);
  EXPECT_GE(msg.first, 0);
  EXPECT_EQ(msg.second, "s: message (first)");
  msg = get_elapsed_time_and_message(lines[3]);
  EXPECT_GE(msg.first, 0);
  EXPECT_EQ(msg.second, "s: FINISHED (second)");
  msg = get_elapsed_time_and_message(lines[4]);
  EXPECT_GE(msg.first, 0);
  EXPECT_EQ(msg.second, "s: FINISHED (first)");
}


}
