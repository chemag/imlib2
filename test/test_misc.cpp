#include <gtest/gtest.h>

#include "config.h"
#include <Imlib2.h>

#include "test.h"

TEST(MISC, version)
{
   EXPECT_EQ(imlib_version(), IMLIB2_VERSION);
}

TEST(MISC, strerror)
{
   if (debug <= 0)
      return;

   for (int i = -10; i < 140; i++)
     {
        printf("%3d: '%s'\n", i, imlib_strerror(i));
     }
}
