#include <iostream>

#include <gtest/gtest.h>

extern "C" {
#include "../src/S16FS.c"
}

unsigned int score;
unsigned int max;

class GradeEnvironment : public testing::Environment {
  public:
    virtual void SetUp() {
        score = 0;
        max   = 60;  // haha, should add this in case people start freaking out where the remiaining is
    }
    virtual void TearDown() { std::cout << "SCORE: " << score << " (out of " << max << ")" << std::endl; }
};

/*

S16FS_t * fs_format(const char *const fname);
    1   Normal
    2   NULL
    3   Empty string

S16FS_t *fs_mount(const char *const fname);
    1   Normal
    2   NULL
    3   Empty string

int fs_unmount(S16FS_t *fs);
    1   Normal
    2   NULL

*/

TEST(a_tests, format_mount_unmount) {
    const char *test_fname = "a_tests.s16fs";

    S16FS_t *fs = NULL;

    // FORMAT 2
    ASSERT_EQ(fs_format(NULL), nullptr);

    // FORMAT 3
    // this really should just be caught by back_store
    ASSERT_EQ(fs_format(""), nullptr);

    // FORMAT 1
    fs = fs_format(test_fname);
    ASSERT_NE(fs, nullptr);

    // UNMOUNT 1
    ASSERT_EQ(fs_unmount(fs), 0);

    // UNMOUNT 2
    ASSERT_LT(fs_unmount(NULL), 0);

    // MOUNT 1
    fs = fs_mount(test_fname);
    ASSERT_NE(fs, nullptr);
    fs_unmount(fs);

    // MOUNT 2
    ASSERT_EQ(fs_mount(NULL), nullptr);

    // MOUNT 3
    // If you get weird behavior here, update/reinstall back_store if you haven't
    // There was a bug where it would try to open files with O_CREATE
    // Which, obviously, would cause issues
    ASSERT_EQ(fs_mount(""), nullptr);

    score += 15;
}

/*

int fs_create(S16FS_t *const fs, const char *const fname, const ftype_t ftype);
    1. Normal, file, in root
    2. Normal, directory, in root
    3. Normal, file, not in root
    4. Normal, directory, not in root
    5. Error, NULL fs
    6. Error, NULL fname
    7. Error, empty fname
    8. Error, bad type
    9. Error, path does not exist
    10. Error, Root clobber
    11. Error, already exists
    12. Error, file exists
    13. Error, part of path not directory
    14. Error, path terminal not directory
    15. Error, path string has no leading slash
    16. Error, path has trailing slash (no name for desired file)
    17. Error, bad path, path part too long
    18. Error, bad path, desired filename too long
    19. Error, directory full.
    20. Error, out of inodes.
    21. Error, out of data blocks & file is directory (requires functional write)

*/

TEST(b_tests, file_creation_one) {
    const char *(filenames[13])
        = {"/file", "/folder", "/folder/with_file", "/folder/with_folder", "/DOESNOTEXIST", "/file/BAD_REQUEST",
           "/DOESNOTEXIST/with_file", "/folder/with_file/bad_req", "folder/missing_slash", "/folder/new_folder/",
           "/folder/withwaytoolongfilenamethattakesupmorespacethanitshould and yet was not enough so I had to add "
           "more/bad_req",
           "/folder/withfilethatiswayyyyytoolongwhydoyoumakefilesthataretoobigEXACT!", "/"};

    const char *test_fname = "b_tests_normal.s16fs";

    S16FS_t *fs = fs_format(test_fname);

    ASSERT_NE(fs, nullptr);

    // CREATE_FILE 1
    ASSERT_EQ(fs_create(fs, filenames[0], FS_REGULAR), 0);

    // CREATE_FILE 2
    ASSERT_EQ(fs_create(fs, filenames[1], FS_DIRECTORY), 0);

    // CREATE_FILE 3
    ASSERT_EQ(fs_create(fs, filenames[2], FS_REGULAR), 0);

    // CREATE_FILE 4
    ASSERT_EQ(fs_create(fs, filenames[3], FS_DIRECTORY), 0);

    // CREATE_FILE 5
    ASSERT_LT(fs_create(NULL, filenames[4], FS_REGULAR), 0);

    // CREATE_FILE 6
    ASSERT_LT(fs_create(fs, NULL, FS_REGULAR), 0);

    // CREATE_FILE 7
    ASSERT_LT(fs_create(fs, "", FS_REGULAR), 0);

    // CREATE_FILE 8
    ASSERT_LT(fs_create(fs, "", (file_t) 44), 0);

    // CREATE_FILE 9
    ASSERT_LT(fs_create(fs, filenames[6], FS_REGULAR), 0);

    // CREATE_FILE 10
    ASSERT_LT(fs_create(fs, filenames[12], FS_DIRECTORY), 0);

    // CREATE_FILE 11
    ASSERT_LT(fs_create(fs, filenames[1], FS_DIRECTORY), 0);
    ASSERT_LT(fs_create(fs, filenames[1], FS_REGULAR), 0);

    // CREATE_FILE 12
    ASSERT_LT(fs_create(fs, filenames[0], FS_REGULAR), 0);
    ASSERT_LT(fs_create(fs, filenames[0], FS_DIRECTORY), 0);

    // CREATE_FILE 13
    ASSERT_LT(fs_create(fs, filenames[5], FS_REGULAR), 0);

    // CREATE_FILE 14
    ASSERT_LT(fs_create(fs, filenames[7], FS_REGULAR), 0);

    // CREATE_FILE 15
    ASSERT_LT(fs_create(fs, filenames[8], FS_REGULAR), 0);
    // But if we don't support relative paths, is there a reason to force abolute notation?
    // It's really a semi-arbitrary restriction
    // I suppose relative paths are up to the implementation, since . and .. are just special folder entires
    // but that would mess with the directory content total, BUT extra parsing can work around that.
    // Hmmmm.

    // CREATE_FILE 16
    ASSERT_LT(fs_create(fs, filenames[9], FS_DIRECTORY), 0);

    // CREATE_FILE 17
    ASSERT_LT(fs_create(fs, filenames[10], FS_REGULAR), 0);

    // CREATE_FILE 18
    ASSERT_LT(fs_create(fs, filenames[11], FS_REGULAR), 0);

    // Closing this file now for inspection to make sure these tests didn't mess it up

    fs_unmount(fs);

    score += 30;
}

TEST(b_tests, file_creation_two) {
    // CREATE_FILE 19 - OUT OF INODES (and test 18 along the way)
    // Gotta make... Uhh... A bunch of files. (255, but we'll need directories to hold them as well)

    const char *test_fname = "b_tests_full_table.s16fs";
    S16FS_t *fs            = fs_format(test_fname);

    ASSERT_NE(fs, nullptr);

    // puts("Attempting to fill inode table...");

    // Dummy string to loop with
    char fname[] = "/a/a\0\0\0\0\0\0\0\0\0\0\0";  // extra space because this is all sorts of messed up now
    // If we do basic a-z, with a-z contained in each, that's... 26*15 which is ~1.5x as much as we need
    // 16 dirs of 15 fills... goes over by one. Ugh.
    // Oh man, AND we run out of space in root.
    // That's annoying.
    for (char dir = 'a'; dir < 'o'; fname[1] = ++dir) {
        fname[2] = '\0';
        ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);
        // printf("File: %s\n", fname);
        fname[2] = '/';
        for (char file = 'a'; file < 'p';) {
            fname[3] = file++;
            // printf("File: %s\n", fname);
            ASSERT_EQ(fs_create(fs, fname, FS_REGULAR), 0);
        }
    }

    // CREATE_FILE 19
    ASSERT_LT(fs_create(fs, "/a/z", FS_REGULAR), 0);
    // Catch up to finish creation
    fname[2] = '\0';
    // printf("File: %s\n", fname);
    ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);
    fname[2] = '/';
    for (char file = 'a'; file < 'o';) {
        fname[3] = file++;
        // printf("File: %s\n", fname);
        ASSERT_EQ(fs_create(fs, fname, FS_REGULAR), 0);
    }
    // ok, need to make /o/o a directory
    fname[3] = 'o';
    // printf("File: %s\n", fname);
    ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);
    // Now there's room for... Ugh, just run it till it breaks and fix it there
    // (/o/o/o apparently)
    // but THAT doesn't work because then we're at a full directory again
    // So we can't test that it failed because we're out of inodes.
    // So we have to make ANOTHER subdirectory.
    // UGGGGhhhhhhhhhhhhhhhhhhh
    fname[4] = '/';
    for (char file = 'a'; file < 'o';) {
        fname[5] = file++;
        // printf("File: %s\n", fname);
        ASSERT_EQ(fs_create(fs, fname, FS_REGULAR), 0);
    }

    fname[5] = 'o';
    // printf("File: %s\n", fname);
    ASSERT_EQ(fs_create(fs, fname, FS_DIRECTORY), 0);

    // Now. Now we are done. No more. Full table.

    // puts("Inode table full?");

    // CREATE_FILE 20
    fname[6] = '/';
    fname[7] = 'a';
    ASSERT_LT(fs_create(fs, fname, FS_REGULAR), 0);
    // save file for inspection
    fs_unmount(fs);

    score += 15;

    // ... Can't really test 21 yet.
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
    return RUN_ALL_TESTS();
}
