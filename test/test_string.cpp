#include <gtest/gtest.h>

/**INDENT-OFF**/
extern "C" {
#include "strutils.h"
}
/**INDENT-ON**/

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

TEST(STR, strsplit_1)
{
   char              **sl;

   sl = __imlib_StrSplit(NULL, '\0');
   ASSERT_FALSE(sl);

   sl = __imlib_StrSplit(NULL, ':');
   ASSERT_FALSE(sl);

   sl = __imlib_StrSplit("abc", '\0');
   ASSERT_FALSE(sl);
}

TEST(STR, strsplit_2)
{
   char              **sl;

   sl = __imlib_StrSplit("", ':');
   ASSERT_FALSE(sl);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit(":", ':');
   ASSERT_FALSE(sl);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit("::", ':');
   ASSERT_FALSE(sl);
   __imlib_StrSplitFree(sl);
}

TEST(STR, strsplit_3)
{
   char              **sl;

   sl = __imlib_StrSplit("abc", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], NULL);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit("abc:", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], NULL);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit(":abc", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], NULL);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit(":abc:", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], NULL);
   __imlib_StrSplitFree(sl);
}

TEST(STR, strsplit_4)
{
   char              **sl;

   sl = __imlib_StrSplit("abc:def", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], "def");
   EXPECT_STREQ(sl[2], NULL);
   __imlib_StrSplitFree(sl);

   sl = __imlib_StrSplit("::abc:::def::", ':');
   ASSERT_TRUE(sl);
   EXPECT_STREQ(sl[0], "abc");
   EXPECT_STREQ(sl[1], "def");
   EXPECT_STREQ(sl[2], NULL);
   __imlib_StrSplitFree(sl);
}
