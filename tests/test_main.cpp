#include <gtest/gtest.h>
#include <glib-object.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  g_type_init();

  return RUN_ALL_TESTS();
}
