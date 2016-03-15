#include <iostream>
#include <cstddef>
#include <cstring>
#include "gtest/gtest.h"

// Using a C library requires extern "C" to prevent function managling
extern "C" {
#include "../src/back_store.c"
}

unsigned int score;

class GradeEnvironment : public testing::Environment {
  public:
    // Override this to define how to set up the environment.
    virtual void SetUp() { score = 0; }
    // Override this to define how to tear down the environment.
    virtual void TearDown() { std::cout << "SCORE: " << score << std::endl; }
};

TEST(bs_create_open, null_fname) {
    back_store_t *res = back_store_create(NULL);
    ASSERT_EQ(nullptr, res);

    res = back_store_open(NULL);
    ASSERT_EQ(nullptr, res);

    score += 2;
}

TEST(bs_create_close, basic) {
    back_store_t *res = back_store_create("test_a.bs");
    ASSERT_NE(nullptr, res);

    back_store_close(res);

    score += 2;
}

TEST(bs_destroy, null_object) {
    back_store_close(NULL);
    // Congrats, you didn't crash!
    score += 2;
}


TEST(bs_read, bad_values) {
    back_store_t *bs = back_store_create("test_c.bs");

    ASSERT_NE(nullptr, bs);

    uint8_t block[1024];

    // make sure we can't touch the FBM
    for (unsigned i = 0; i < 8; ++i) {
        ASSERT_FALSE(back_store_read(bs, i, block));
    }

    unsigned block_a = back_store_allocate(bs);

    ASSERT_FALSE(back_store_read(bs, block_a, NULL));

    ASSERT_FALSE(back_store_read(NULL, block_a, block));

    back_store_close(bs);

    score += 6;
}

TEST(bs_write, bad_values) {
    back_store_t *bs = back_store_create("test_d.bs");

    ASSERT_NE(nullptr, bs);

    uint8_t block[1024];

    // make sure we can't touch the FBM
    for (unsigned i = 0; i < 8; ++i) {
        ASSERT_FALSE(back_store_write(bs, i, block));
    }

    unsigned block_a = back_store_allocate(bs);

    ASSERT_FALSE(back_store_write(bs, block_a, NULL));

    ASSERT_FALSE(back_store_write(NULL, block_a, block));

    back_store_close(bs);

    score += 6;
}

TEST(bs_a_lot, basic_use) {
    // pretty much do everything
    // read: did you save the fbm changes? data?
    back_store_t *bs = back_store_create("test_b.bs");
    ASSERT_NE(nullptr, bs);

    unsigned block_a = back_store_allocate(bs);
    ASSERT_NE(0, block_a);

    unsigned block_b = 12;
    ASSERT_TRUE(back_store_request(bs, block_b));

    uint8_t data_blocks[4][1024];
    memset(data_blocks[0], 0x05, 1024);
    memset(data_blocks[1], 0xFF, 1024);
    memset(data_blocks[2], 0x00, 1024);

    ASSERT_TRUE(back_store_read(bs, block_a, data_blocks[3]));

    // was it zero-init'd?
    ASSERT_EQ(0, memcmp(data_blocks[2], data_blocks[3], 1024));

    ASSERT_TRUE(back_store_read(bs, block_b, data_blocks[3]));

    // was it zero-init'd? Did you just get lucky last time?
    ASSERT_EQ(0, memcmp(data_blocks[2], data_blocks[3], 1024));

    // write data out
    ASSERT_TRUE(back_store_write(bs, block_a, data_blocks[0]));
    ASSERT_TRUE(back_store_write(bs, block_b, data_blocks[1]));

    back_store_close(bs);

    bs = back_store_open("test_b.bs");

    // Did the FBM get saved?
    ASSERT_FALSE(back_store_request(bs, block_a));
    ASSERT_FALSE(back_store_request(bs, block_b));

    // was the data saved?
    ASSERT_TRUE(back_store_read(bs, block_a, data_blocks[3]));
    ASSERT_EQ(0, memcmp(data_blocks[0], data_blocks[3], 1024));

    ASSERT_TRUE(back_store_read(bs, block_b, data_blocks[3]));
    ASSERT_EQ(0, memcmp(data_blocks[1], data_blocks[3], 1024));

    // free then re-request
    back_store_release(bs, block_a);
    back_store_release(bs, block_b);
    ASSERT_TRUE(back_store_request(bs, block_a));
    ASSERT_TRUE(back_store_request(bs, block_b));

    back_store_close(bs);

    score += 35;
}

TEST(bs_request_release, fbm_attack) {
    uint8_t buffer[1024];
    back_store_t *bs = back_store_create("test_k.bs");  // I forgot what letter I was on
    for (unsigned i = 0; i < 8; ++i) {
        back_store_release(bs, i);
        ASSERT_FALSE(back_store_request(bs, i));
        ASSERT_FALSE(back_store_read(bs,i,buffer));
        ASSERT_FALSE(back_store_write(bs,i,buffer));
    }
    back_store_close(bs);
    score += 6;
}

TEST(bs_request, fill_device) {
    back_store_t *bs = back_store_create("test_l.bs");
    for (unsigned i = 8; i < 65536; ++i) {
        ASSERT_TRUE(back_store_request(bs, i));
    }
    ASSERT_EQ(back_store_allocate(bs), 0);
    back_store_close(bs);
    score += 6;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
    return RUN_ALL_TESTS();
}
