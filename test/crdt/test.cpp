#include <gtest/gtest.h>

#include "../../src/crdt/counter.h"
#include "../../src/crdt/join.h"

class BaseCounterTest : public ::testing::Test {
   protected:
    BaseCounter<> u1{"user1"};
    BaseCounter<> u2{"user2"};

    void SetUp() override {
    }
};

TEST_F(BaseCounterTest, InitialValuesAreZero) {
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u1.local(), 0);
}

TEST_F(BaseCounterTest, IncrementUpdatesLocalAndRead) {
    u1.inc();
    EXPECT_EQ(u1.local(), 1);
    EXPECT_EQ(u1.read(), 1);
}

TEST_F(BaseCounterTest, IncrementDifferentValues) {
    u1.inc(0);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.inc(10);
    EXPECT_EQ(u1.local(), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.inc(-10);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(BaseCounterTest, JoiningCounters) {
    u1.inc();
    u2.inc();
    u1.join(u2);
    EXPECT_EQ(u1.local(), 1);
    EXPECT_EQ(u1.read(), 2);
    EXPECT_EQ(u2.local(), 1);
    EXPECT_EQ(u2.read(), 1);
}

TEST_F(BaseCounterTest, UsingDeltas) {
    BaseCounter<> delta_u1, delta_u2;
    delta_u1.join(u1.inc());
    EXPECT_EQ(delta_u1.local(), 0);
    EXPECT_EQ(delta_u1.read(), 1);

    delta_u1.join(u1.inc(4));
    EXPECT_EQ(delta_u1.read(), 5);

    delta_u2.join(u2.inc());
    delta_u2.join(u2.inc());

    BaseCounter<> u3 = join(u1, u2);
    EXPECT_EQ(u3.read(), 7);
    EXPECT_EQ(u3.local(), 5);  // Join makes a copy of u1 and joins with u2

    BaseCounter<> u4 = join(join(u1, delta_u2), join(u2, delta_u1));
    EXPECT_EQ(u4.read(), 7);
    EXPECT_EQ(u4.local(), 5);  // Join makes a copy of u1 and joins with u2

    BaseCounter<> u5 = join(u1, delta_u2);
    EXPECT_EQ(u5.read(), 7);
    EXPECT_EQ(u5.local(), 5);  // Join makes a copy of u1 and joins with u2
}

class CounterTest : public ::testing::Test {
   protected:
    Counter<> u1{"user1"};
    Counter<> u2{"user2"};

    void SetUp() override {
    }
};

TEST_F(CounterTest, InitialValuesAreZero) {
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u1.local(), 0);
}

TEST_F(CounterTest, IncrementUpdatesLocalAndRead) {
    u1.inc();
    EXPECT_EQ(u1.local(), 1);
    EXPECT_EQ(u1.read(), 1);
}

TEST_F(CounterTest, IncrementDifferentValues) {
    u1.inc(0);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.inc(10);
    EXPECT_EQ(u1.local(), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.inc(-10);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, DecrementUpdatesLocalAndRead) {
    u1.dec();
    EXPECT_EQ(u1.local(), -1);
    EXPECT_EQ(u1.read(), -1);
}

TEST_F(CounterTest, DecrementDifferentValues) {
    u1.dec(0);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.dec(10);
    EXPECT_EQ(u1.local(), -10);
    EXPECT_EQ(u1.read(), -10);

    u1.dec(-10);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, IncrementAndDecrement) {
    u1.inc(10);
    EXPECT_EQ(u1.local(), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.dec(10);
    EXPECT_EQ(u1.local(), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, JoiningCounters) {
    u1.inc();
    u2.dec();
    u1.join(u2);
    EXPECT_EQ(u1.local(), 1);
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u2.local(), -1);
    EXPECT_EQ(u2.read(), -1);
}

TEST_F(CounterTest, UsingDeltas) {
    Counter<> delta_u1, delta_u2;
    delta_u1.join(u1.inc());
    EXPECT_EQ(delta_u1.local(), 0);
    EXPECT_EQ(delta_u1.read(), 1);

    delta_u1.join(u1.dec(4));
    EXPECT_EQ(delta_u1.read(), -3);

    delta_u2.join(u2.inc(3));
    delta_u2.join(u2.inc());

    Counter<> u3 = join(u1, u2);
    EXPECT_EQ(u3.read(), 1);
    EXPECT_EQ(u3.local(), -3);  // Join makes a copy of u1 and joins with u2

    Counter<> u4 = join(join(u1, delta_u2), join(u2, delta_u1));
    EXPECT_EQ(u4.read(), 1);
    EXPECT_EQ(u4.local(), -3);  // Join makes a copy of u1 and joins with u2

    Counter<> u5 = join(u1, delta_u2);
    EXPECT_EQ(u5.read(), 1);
    EXPECT_EQ(u5.local(), -3);  // Join makes a copy of u1 and joins with u2
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
