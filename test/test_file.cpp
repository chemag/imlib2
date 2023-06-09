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
void                __imlib_FileDel(const char *s);
time_t              __imlib_FileModDate(const char *s);
char               *__imlib_FileHomeDir(int uid);
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

TEST(FILE, file_exists)
{
   int                 rc;

   rc = __imlib_FileExists("./Makefile");
   EXPECT_EQ(rc, 1);

   rc = __imlib_FileExists(".");
   EXPECT_EQ(rc, 1);

   rc = __imlib_FileExists("./foob");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileExists("./Makefile:foo");
   EXPECT_EQ(rc, USE_REAL_FILE);

   rc = __imlib_FileExists(".:foo");
   EXPECT_EQ(rc, USE_REAL_FILE);

   rc = __imlib_FileExists("./foob:foo");
   EXPECT_EQ(rc, 0);
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

TEST(FILE, file_is_dir)
{
   int                 rc;

   rc = __imlib_FileIsDir("./Makefile");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsDir(".");
   EXPECT_EQ(rc, 1);

   rc = __imlib_FileIsDir("./foob");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsDir("./Makefile:foo");
   EXPECT_EQ(rc, 0);

   rc = __imlib_FileIsDir(".:foo");
   EXPECT_EQ(rc, USE_REAL_FILE);

   rc = __imlib_FileIsDir("./foob:foo");
   EXPECT_EQ(rc, 0);
}

TEST(FILE, file_is_real_file)
{
   int                 rc;

   rc = __imlib_IsRealFile("./Makefile");
   EXPECT_EQ(rc, 1);

   rc = __imlib_IsRealFile(".");
   EXPECT_EQ(rc, 0);

   rc = __imlib_IsRealFile("./foob");
   EXPECT_EQ(rc, 0);

   rc = system("touch gylle");
   EXPECT_EQ(rc, 0);
   rc = __imlib_IsRealFile("gylle");
   EXPECT_EQ(rc, 1);

   rc = system("chmod 000 gylle");
   EXPECT_EQ(rc, 0);
   rc = __imlib_IsRealFile("gylle");
   EXPECT_EQ(rc, 1);

   rc = unlink("gylle");
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
