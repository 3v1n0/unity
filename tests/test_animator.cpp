#include <gtest/gtest.h>

#include "Animator.h"

#include "test_utils.h"

using namespace std;
using namespace unity;

namespace
{

class TestAnimator : public ::testing::Test
{
public:
  TestAnimator() :
   test_animator_(Animator(100)),
   n_steps_(0),
   current_progress_(0.0f),
   started_(false),
   stopped_(false),
   ended_(false)
  {
    test_animator_.animation_started.connect([&started_]() {
      started_ = true;
    });

    test_animator_.animation_ended.connect([&ended_]() {
      ended_ = true;
    });

    test_animator_.animation_stopped.connect([&stopped_](double progress) {
      stopped_ = true;
    });

    test_animator_.animation_updated.connect([&n_steps_, &current_progress_](double progress) {
      n_steps_++;
      current_progress_ = progress;
    });
  }

  void ResetValues()
  {
    n_steps_ = 0;
    current_progress_ = 0.0f;
    started_ = false;
    stopped_ = false;
    ended_ = false;
  }

  Animator test_animator_;
  unsigned int n_steps_;
  double current_progress_;
  bool started_;
  bool stopped_;
  bool ended_;
};

TEST_F(TestAnimator, ConstructDestroy)
{
  auto tmp_animator = new Animator(200, 25);

  EXPECT_EQ(tmp_animator->GetDuration(), 200);
  EXPECT_EQ(tmp_animator->GetRate(), 25);

  double progress;
  tmp_animator->animation_updated.connect([&progress](double p) {
    progress = p;
  });

  tmp_animator->Start();

  EXPECT_EQ(tmp_animator->IsRunning(), true);
  EXPECT_GT(progress, 0.0f);

  delete tmp_animator;
}

TEST_F(TestAnimator, SetGetValues)
{
  test_animator_.SetRate(30);
  EXPECT_EQ(test_animator_.GetRate(), 30);

  test_animator_.SetDuration(100);
  EXPECT_EQ(test_animator_.GetDuration(), 100);

  EXPECT_EQ(test_animator_.GetProgress(), 0.0f);
  EXPECT_EQ(test_animator_.IsRunning(), false);
}

TEST_F(TestAnimator, SimulateStep)
{
  test_animator_.DoStep();
  EXPECT_EQ(test_animator_.IsRunning(), false);
  EXPECT_GT(test_animator_.GetProgress(), 0.0f);
  ResetValues();
}

} // Namespace
