#include <gtest/gtest.h>

/**INDENT-OFF**/
extern "C" {
#include "file.h"
}
/**INDENT-ON**/

#define EXPECT_OK(x)  EXPECT_FALSE(x)
#define EXPECT_ERR(x) EXPECT_TRUE(x)

#if 0
char               *__imlib_FileRealFile(const char *file);

char              **__imlib_FileDir(const char *dir, int *num);
void                __imlib_FileFreeDirList(char **l, int num);
time_t              __imlib_FileModDate(const char *s);
int                 __imlib_FilePermissions(const char *s);
#endif

#define USE_REAL_FILE 0

TEST(FILE, file_extension)
{
   const char         *s;

   s = __imlib_FileExtension("abc.def");
   EXPECT_STREQ(s, "def");

   s = __imlib_FileExtension(".def");
   EXPECT_STREQ(s, "def");

   s = __imlib_FileExtension("def");
   EXPECT_STREQ(s, "def");

   s = __imlib_FileExtension(".");
   EXPECT_STREQ(s, NULL);

   s = __imlib_FileExtension("def.");
   EXPECT_STREQ(s, NULL);

   s = __imlib_FileExtension("/def.x");
   EXPECT_STREQ(s, "x");

   s = __imlib_FileExtension("/def");
   EXPECT_STREQ(s, "def");
}

TEST(FILE, file_is_file)
{
   int                 rc;

   rc = __imlib_FileIsFile("./Makefile");
   EXPECT_EQ(rc, 1);

   rc = __imlib_FileIsFile(".");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsFile("./foob");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsFile("./Makefile:foo");
   EXPECT_EQ(rc, USE_REAL_FILE);

   rc = __imlib_FileIsFile(".:foo");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsFile("./foob:foo");
   EXPECT_EQ(rc, 0);
}

TEST(FILE, file_key)
{
   char               *key;

   key = __imlib_FileKey("file.ext:key");
   EXPECT_STREQ(key, "key");
   free(key);

   key = __imlib_FileKey("file.ext:key=abc");
   EXPECT_STREQ(key, "key=abc");
   free(key);

   key = __imlib_FileKey("file.ext:key:abc");
   EXPECT_STREQ(key, "key:abc");
   free(key);

   key = __imlib_FileKey("file.ext:key:");
   EXPECT_STREQ(key, "key:");
   free(key);

   key = __imlib_FileKey("file.ext:");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("file.ext");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("file");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("file.ext::key");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("C::file.ext:key");
   EXPECT_STREQ(key, "key");
   free(key);

   key = __imlib_FileKey("Drive::file.ext:key:zz");
   EXPECT_STREQ(key, "key:zz");
   free(key);

   key = __imlib_FileKey("C::file.ext:");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("C::file.ext");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("C::");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("C:::");
   EXPECT_FALSE(key);
   free(key);

   key = __imlib_FileKey("::C:");
   EXPECT_FALSE(key);
   free(key);
}
