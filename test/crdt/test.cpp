#include <gtest/gtest.h>

#include <iostream>
#include <set>

#include "../../src/crdt/counter.h"
#include "../../src/crdt/gset.h"
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

class GSetTest : public ::testing::Test {
   protected:
    GSet<> u1{"user1"};
    GSet<> u2{"user2"};

    void SetUp() override {
    }
};

TEST_F(GSetTest, InitialEmpty) {
    EXPECT_EQ(u1.elements().size(), 0);
    EXPECT_FALSE(u1.contains("Banana"));
    EXPECT_FALSE(u1.get("Banana").second);
}

TEST_F(GSetTest, AddElement) {
    u1.add("Banana");
    EXPECT_EQ(u1.elements().size(), 1);
    EXPECT_TRUE(u1.contains("Banana"));
    auto p = u1.get("Banana");
    EXPECT_TRUE(p.second);
    EXPECT_EQ(p.first->read(), 0);
    u1.add("Tomato");
    EXPECT_EQ(u1.elements().size(), 2);
    EXPECT_TRUE(u1.contains("Tomato"));
    p = u1.get("Tomato");
    EXPECT_TRUE(p.second);
    EXPECT_EQ(p.first->read(), 0);
}

TEST_F(GSetTest, GetElement) {
    u1.add("Banana");
    auto p = u1.get("Banana");
    EXPECT_TRUE(p.second);
    EXPECT_EQ(p.first->read(), 0);
    p.first->inc(10);
    EXPECT_EQ(u1.get("Banana").first->read(), 10);
}

TEST_F(GSetTest, AddDoesNotOverwrite) {
    u1.add("Banana");
    auto p = u1.get("Banana");
    EXPECT_TRUE(p.second);
    EXPECT_EQ(p.first->read(), 0);
    p.first->inc(10);
    EXPECT_EQ(p.first->read(), 10);

    u1.add("Banana");
    EXPECT_EQ(p.first->read(), 10);
    EXPECT_EQ(u1.get("Banana").first->read(), 10);
}

TEST_F(GSetTest, CheckElements) {
    std::set<std::string> fruits = {"Banana", "Tomato", "Pineapple"};
    for (const auto& f : fruits) {
        u1.add(f);
    }

    EXPECT_EQ(u1.elements(), fruits);
}

TEST_F(GSetTest, JoiningGSets) {
    u1.add("Banana");
    u1.add("Pineapple");
    u2.add("Tomato");
    u2.add("Banana");

    u1.get("Banana").first->inc();
    u2.get("Banana").first->inc();

    u1.join(u2);
    std::set<std::string> fruits = {"Banana", "Tomato", "Pineapple"};
    EXPECT_EQ(u1.elements(), fruits);
    EXPECT_EQ(u1.get("Banana").first->read(), 2);
    EXPECT_EQ(u1.get("Pineapple").first->read(), 0);
    EXPECT_EQ(u1.get("Tomato").first->read(), 0);

    u1.join(u2);
    EXPECT_EQ(u1.elements(), fruits);
    EXPECT_EQ(u1.get("Banana").first->read(), 2);
    EXPECT_EQ(u1.get("Pineapple").first->read(), 0);
    EXPECT_EQ(u1.get("Tomato").first->read(), 0);
}

TEST_F(GSetTest, UsingIncAndDec) {
    u1.add("Banana");
    u1.inc("Banana");
    EXPECT_EQ(u1.get("Banana").first->read(), 1);
    u1.dec("Banana");
    EXPECT_EQ(u1.get("Banana").first->read(), 0);
}

TEST_F(GSetTest, Read) {
    u1.add("Banana");
    u1.inc("Banana", 2);
    EXPECT_EQ(u1.read("Banana"), 2);
    u1.dec("Banana", 2);
    EXPECT_EQ(u1.read("Banana"), 0);
    EXPECT_EQ(u1.read("Pineapple"), 0);
}

TEST_F(GSetTest, UsingDeltas) {
    GSet<> delta_u1, delta_u2;
    delta_u1.join(u1.add("Banana"));
    EXPECT_TRUE(delta_u1.contains("Banana"));

    delta_u1.join(u1.inc("Banana"));
    EXPECT_EQ(delta_u1.read("Banana"), 1);

    delta_u2.join(u2.add("Pineapple"));
    delta_u2.join(u2.add("Tomato"));
    delta_u2.join(u2.add("Banana"));
    delta_u2.join(u2.dec("Banana", 2));

    std::set<std::string> fruits = {"Banana", "Tomato", "Pineapple"};

    GSet<> u3 = join(u1, u2);
    EXPECT_EQ(u3.elements(), fruits);
    EXPECT_EQ(u3.read("Banana"), -1);
    EXPECT_EQ(u3.read("Tomato"), 0);
    EXPECT_EQ(u3.read("Pineapple"), 0);

    GSet<> u4 = join(join(u1, delta_u2), join(u2, delta_u1));
    EXPECT_EQ(u4.elements(), fruits);
    EXPECT_EQ(u4.read("Banana"), -1);
    EXPECT_EQ(u4.read("Tomato"), 0);
    EXPECT_EQ(u4.read("Pineapple"), 0);

    GSet<> u5 = join(u1, delta_u2);
    EXPECT_EQ(u5.elements(), fruits);
    EXPECT_EQ(u5.read("Banana"), -1);
    EXPECT_EQ(u5.read("Tomato"), 0);
    EXPECT_EQ(u5.read("Pineapple"), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
