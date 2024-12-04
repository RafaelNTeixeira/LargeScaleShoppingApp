#include <gtest/gtest.h>

#include <iostream>
#include <set>

#include "../../src/crdt/counter.h"
#include "../../src/crdt/join.h"
#include "../../src/crdt/new_aworset.h"
#include "../../src/crdt/new_shopping_list.h"

class BaseCounterTest : public ::testing::Test {
   protected:
    BaseCounter<> u1{};
    BaseCounter<> u2{};
    std::string s1 = "user1";
    std::string s2 = "user2";

    void SetUp() override {
    }
};

TEST_F(BaseCounterTest, InitialValuesAreZero) {
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u1.local(s1), 0);
}

TEST_F(BaseCounterTest, IncrementUpdatesLocalAndRead) {
    u1.inc(s1);
    EXPECT_EQ(u1.local(s1), 1);
    EXPECT_EQ(u1.local(s2), 0);
    EXPECT_EQ(u1.read(), 1);
}

TEST_F(BaseCounterTest, IncrementDifferentValues) {
    u1.inc(s1, 0);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.inc(s1, -10);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(BaseCounterTest, JoiningCounters) {
    u1.inc(s1);
    u2.inc(s2);
    u1.join(u2);
    EXPECT_EQ(u1.local(s1), 1);
    EXPECT_EQ(u1.read(), 2);
    EXPECT_EQ(u2.local(s2), 1);
    EXPECT_EQ(u2.read(), 1);
}

TEST_F(BaseCounterTest, UsingDeltas) {
    BaseCounter<> delta_u1, delta_u2;
    delta_u1.join(u1.inc(s1));
    EXPECT_EQ(delta_u1.local(s1), 1);
    EXPECT_EQ(delta_u1.read(), 1);

    delta_u1.join(u1.inc(s1, 4));
    EXPECT_EQ(delta_u1.read(), 5);
    EXPECT_EQ(delta_u1.local(s1), 5);

    delta_u2.join(u2.inc(s2));
    delta_u2.join(u2.inc(s2));

    BaseCounter<> u3 = join::join(u1, u2);
    EXPECT_EQ(u3.read(), 7);
    EXPECT_EQ(u3.local(s1), 5);
    EXPECT_EQ(u3.local(s2), 2);

    BaseCounter<> u4 = join::join(join::join(u1, delta_u2), join::join(u2, delta_u1));
    EXPECT_EQ(u4.read(), 7);
    EXPECT_EQ(u4.local(s1), 5);
    EXPECT_EQ(u4.local(s2), 2);

    BaseCounter<> u5 = join::join(u1, delta_u2);
    EXPECT_EQ(u5.read(), 7);
    EXPECT_EQ(u5.local(s1), 5);
    EXPECT_EQ(u5.local(s2), 2);
}

class CounterTest : public ::testing::Test {
   protected:
    Counter<> u1{};
    Counter<> u2{};
    std::string s1 = "user1";
    std::string s2 = "user2";

    void SetUp() override {
    }
};

TEST_F(CounterTest, InitialValuesAreZero) {
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u1.local(s1), 0);
}

TEST_F(CounterTest, IncrementUpdatesLocalAndRead) {
    u1.inc(s1);
    EXPECT_EQ(u1.local(s1), 1);
    EXPECT_EQ(u1.local(s2), 0);
    EXPECT_EQ(u1.read(), 1);
}

TEST_F(CounterTest, IncrementDifferentValues) {
    u1.inc(s1, 0);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.inc(s1, -10);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, DecrementUpdatesLocalAndRead) {
    u1.dec(s1);
    EXPECT_EQ(u1.local(s1), -1);
    EXPECT_EQ(u1.read(), -1);
}

TEST_F(CounterTest, DecrementDifferentValues) {
    u1.dec(s1, 0);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.dec(s1, 10);
    EXPECT_EQ(u1.local(s1), -10);
    EXPECT_EQ(u1.read(), -10);

    u1.dec(s1, -10);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, IncrementAndDecrement) {
    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 10);

    u1.dec(s1, 10);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);
}

TEST_F(CounterTest, JoiningCounters) {
    u1.inc(s1);
    u2.dec(s2);
    u1.join(u2);
    EXPECT_EQ(u1.local(s1), 1);
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u2.local(s2), -1);
    EXPECT_EQ(u2.read(), -1);
}

TEST_F(CounterTest, UsingDeltas) {
    Counter<> delta_u1, delta_u2;
    delta_u1.join(u1.inc(s1));
    EXPECT_EQ(delta_u1.local(s1), 1);
    EXPECT_EQ(delta_u1.read(), 1);

    delta_u1.join(u1.dec(s1, 4));
    EXPECT_EQ(delta_u1.read(), -3);

    delta_u2.join(u2.inc(s2, 3));
    delta_u2.join(u2.inc(s2));

    Counter<> u3 = join::join(u1, u2);
    EXPECT_EQ(u3.read(), 1);
    EXPECT_EQ(u3.local(s1), -3);
    EXPECT_EQ(u3.local(s2), 4);

    Counter<> u4 = join::join(join::join(u1, delta_u2), join::join(u2, delta_u1));
    EXPECT_EQ(u4.read(), 1);
    EXPECT_EQ(u4.local(s1), -3);
    EXPECT_EQ(u4.local(s2), 4);

    Counter<> u5 = join::join(u1, delta_u2);
    EXPECT_EQ(u5.read(), 1);
    EXPECT_EQ(u5.local(s1), -3);
    EXPECT_EQ(u5.local(s2), 4);
}

class DotContextTest : public ::testing::Test {
   protected:
    DotContext u1{};
    DotContext u2{};

    void SetUp() override {
    }
};

TEST_F(DotContextTest, InitialEmpty) {
    EXPECT_EQ(u1.vc.size(), 0);
    EXPECT_EQ(u1.dc.size(), 0);
}

TEST_F(DotContextTest, InsertDot) {
    Dot d = u1.nextDot("user1");

    EXPECT_EQ(u1.vc.size(), 1);
    EXPECT_EQ(u1.dc.size(), 0);
    EXPECT_EQ(d, Dot("user1", 1));
    EXPECT_EQ(u1.contains(d), true);

    Dot d2 = u1.nextDot("user1");

    EXPECT_EQ(u1.vc.size(), 1);
    EXPECT_EQ(u1.dc.size(), 0);
    EXPECT_EQ(d, Dot("user1", 1));
    EXPECT_EQ(u1.contains(d), true);
    EXPECT_EQ(u1.contains(d2), true);
}

TEST_F(DotContextTest, JoiningDotContexts) {
    Dot d1 = u1.nextDot("user1");
    Dot d2 = u2.nextDot("user2");

    u1.join(u2);

    EXPECT_EQ(u1.vc.size(), 2);
    EXPECT_EQ(u1.dc.size(), 0);
    EXPECT_EQ(u1.contains(d1), true);
    EXPECT_EQ(u1.contains(d2), true);
}

TEST_F(DotContextTest, Compact) {
    Dot d1 = Dot("user1", 0);
    Dot d2 = Dot("user1", 1);

    u1.dc.insert(d2);

    EXPECT_EQ(u1.contains(d1), false);
    EXPECT_EQ(u1.contains(d2), true);

    u1.compact();

    EXPECT_EQ(u1.vc.size(), 0);
    EXPECT_EQ(u1.dc.size(), 1);

    u1.dc.insert(d1);

    EXPECT_EQ(u1.contains(d1), true);
    EXPECT_EQ(u1.contains(d2), true);
    u1.compact();

    EXPECT_EQ(u1.vc.size(), 1);
    EXPECT_EQ(u1.dc.size(), 0);
}

class DotKernelTest : public ::testing::Test {
   protected:
    DotKernel<int> u1{};
    DotKernel<int> u2{};
    Dot d1{"user1", 1};
    Dot d2{"user2", 2};

    void SetUp() override {
    }
};

TEST_F(DotKernelTest, InitialEmpty) {
    EXPECT_EQ(u1.values.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 0);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

TEST_F(DotKernelTest, AddDot) {
    u1.add("user1", 10);

    EXPECT_EQ(u1.values.size(), 1);
    EXPECT_EQ(u1.context.vc.size(), 1);
    EXPECT_EQ(u1.context.dc.size(), 0);
    EXPECT_EQ(u1.allValues().find(10) != u1.allValues().end(), true);

    u1.add("user1", 20);

    EXPECT_EQ(u1.values.size(), 2);
    EXPECT_EQ(u1.context.vc.size(), 1);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

TEST_F(DotKernelTest, RemoveDot) {
    u1.add("user1", 10);
    u1.add("user2", 20);

    EXPECT_EQ(u1.values.size(), 2);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);

    u1.remove(10);

    EXPECT_EQ(u1.values.size(), 1);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);

    u1.remove(Dot("user2", 1));

    EXPECT_EQ(u1.values.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);

    u1.add("user1", 10);

    EXPECT_EQ(u1.values.size(), 1);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);

    u1.remove();

    EXPECT_EQ(u1.values.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

TEST_F(DotKernelTest, JoiningDotKernels) {
    u1.add("user1", 10);
    u2.add("user2", 20);

    u1.join(u2);

    EXPECT_EQ(u1.values.size(), 2);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

class CasualCounterTest : public ::testing::Test {
   protected:
    CausalCounter<int> u1{};
    CausalCounter<int> u2{};
    std::string s1 = "user1";
    std::string s2 = "user2";

    void SetUp() override {
    }
};

TEST_F(CasualCounterTest, InitialEmpty) {
    EXPECT_EQ(u1.read(), 0);
    EXPECT_EQ(u1.local(s1), 0);
}

TEST_F(CasualCounterTest, IncrementUpdatesLocalAndRead) {
    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.local(s2), 0);
    EXPECT_EQ(u1.read(), 10);
}

TEST_F(CasualCounterTest, IncrementDifferentValues) {
    u1.inc(s1, 0);
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.read(), 0);

    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 10);
}

TEST_F(CasualCounterTest, DecrementUpdatesLocalAndRead) {
    u1.inc(s1, 100);
    u1.dec(s1, 10);

    EXPECT_EQ(u1.local(s1), 90);
    EXPECT_EQ(u1.read(), 90);

    u1.dec(s1, 100);
    EXPECT_EQ(u1.local(s1), -10);
    EXPECT_EQ(u1.read(), -10);
    u1.dec(s1);
    EXPECT_EQ(u1.local(s1), -1);
    EXPECT_EQ(u1.read(), -1);
    u1.inc(s1, 10);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 10);
}

TEST_F(CasualCounterTest, JoiningCounters) {
    u1.inc(s1, 10);
    u2.inc(s2, 20);
    u1.join(u2);
    EXPECT_EQ(u1.local(s1), 10);
    EXPECT_EQ(u1.read(), 30);
    EXPECT_EQ(u2.local(s2), 20);
    EXPECT_EQ(u2.local(s2), 20);
    EXPECT_EQ(u2.read(), 20);
}

TEST_F(CasualCounterTest, Reset) {
    u1.inc(s1, 10);
    u1.inc(s2, 20);
    u1.reset();
    EXPECT_EQ(u1.local(s1), 0);
    EXPECT_EQ(u1.local(s2), 0);
    EXPECT_EQ(u1.read(), 0);
}

class ORMapTest : public ::testing::Test {
   protected:
    ORMap<std::string, CausalCounter<int>> u1{};
    ORMap<std::string, CausalCounter<int>> u2{};
    std::string s1 = "banana";
    std::string s2 = "apple";
    std::string i1 = "user1";
    std::string i2 = "user2";

    void SetUp() override {
    }
};

TEST_F(ORMapTest, InitialEmpty) {
    EXPECT_EQ(u1.map.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 0);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

TEST_F(ORMapTest, ModifyValues) {
    u1[s1].inc(i1, 10);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 1);
    EXPECT_EQ(u1[s1].read(), 10);
    u1[s1].inc(i1, 10);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 2);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 2);
    EXPECT_EQ(u1[s1].read(), 20);
    u1[s1].dec(i1, 10);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 3);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 3);
    EXPECT_EQ(u1[s1].read(), 10);

    u1[s2].inc(i1, 20);
    EXPECT_EQ(u1.map.size(), 2);
    EXPECT_EQ(u1.context.vc[i1], 4);
    EXPECT_EQ(u1[s2].core.context.vc[i1], 4);
    EXPECT_EQ(u1[s2].read(), 20);

    u1[s1].inc(i2, 10);
    EXPECT_EQ(u1.map.size(), 2);
    EXPECT_EQ(u1.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].read(), 20);
}

TEST_F(ORMapTest, Remove) {
    u1[s1].inc(i1, 10);
    u1[s2].inc(i2, 20);
    u1.remove(s1);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);
    u1.remove(s2);
    EXPECT_EQ(u1.map.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);
}

TEST_F(ORMapTest, Reset) {
    u1[s1].inc(i1, 10);
    u1[s2].inc(i2, 20);
    u1.reset();
    EXPECT_EQ(u1.map.size(), 0);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.dc.size(), 0);

    u1[s1].inc(i1, 10);

    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc.size(), 2);
    EXPECT_EQ(u1.context.vc[i1], 2);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 2);
}

TEST_F(ORMapTest, JoiningMaps) {
    u1[s1].inc(i1, 10);
    u2[s2].inc(i2, 20);
    u1.join(u2);
    EXPECT_EQ(u1.map.size(), 2);
    EXPECT_EQ(u1.context.vc[i1], 1);
    EXPECT_EQ(u1.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 1);
    EXPECT_EQ(u1[s2].core.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].read(), 10);
    EXPECT_EQ(u1[s2].read(), 20);
}

TEST_F(ORMapTest, JoiningWithConflicts) {
    u1[s1].inc(i1, 10);
    u2[s1].inc(i2, 20);
    u1.join(u2);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 1);
    EXPECT_EQ(u1.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].read(), 30);

    u1[s1].inc(i1, 10);
    u2[s1].inc(i2, 20);
    u2[s1].inc(i2, 20);
    u1.join(u2);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 2);
    EXPECT_EQ(u1.context.vc[i2], 3);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 2);
    EXPECT_EQ(u1[s1].core.context.vc[i2], 3);
    EXPECT_EQ(u1[s1].read(), 80);
}

TEST_F(ORMapTest, JoiningIncRemove) {
    u1[s1].inc(i1, 10);
    u2[s1].inc(i2, 20);
    u2.remove(s1);
    u1.join(u2);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1.context.vc[i1], 1);
    EXPECT_EQ(u1.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i1], 1);
    EXPECT_EQ(u1[s1].core.context.vc[i2], 1);
    EXPECT_EQ(u1[s1].read(), 10);

    u2.join(u1);
    EXPECT_EQ(u2.map.size(), 1);
    EXPECT_EQ(u2[s1].read(), 10);

    u2.remove(s1);
    u1.join(u2);
    EXPECT_EQ(u1.map.size(), 1);
    EXPECT_EQ(u1[s1].core.values.size(), 0);

    u1[s1].inc(i1, 10);
    EXPECT_EQ(u1[s1].read(), 10);

    u2[s1].inc(i2, 20);
    u1.join(u2);
    EXPECT_EQ(u1[s1].read(), 30);

    u2.remove(s1);
    EXPECT_EQ(u2[s1].read(), 0);

    u1.join(u2);
    EXPECT_EQ(u1[s1].read(), 10);
}

class ShoppingListTest : public ::testing::Test {
   protected:
    ShoppingList u1{"user1", "Shopping List", "http://example.com"};
    ShoppingList u2{"user2", "Shopping List", "http://example.com"};
    ShoppingList u3{"user1", "Not Shopping List", "http://example.com"};
    ShoppingList u4{"user1", "Shopping List", "http://not_good_example.com"};

    void SetUp() override {
    }
};

TEST_F(ShoppingListTest, InitialEmpty) {
    EXPECT_EQ(u1.getItemsSet().size(), 0);
    EXPECT_EQ(u1.getId(), "user1");
    EXPECT_EQ(u1.getTitle(), "Shopping List");
    EXPECT_EQ(u1.getURL(), "http://example.com");
}

TEST_F(ShoppingListTest, CreateItem) {
    EXPECT_TRUE(u1.createItem("banana", 10));
    EXPECT_EQ(u1.getItemsSet().size(), 1);
    EXPECT_EQ(u1.getItemsSet().begin()->first, "banana");
    EXPECT_EQ(u1.getItemsSet().begin()->second, 10);

    EXPECT_FALSE(u1.createItem("banana", 10));
}

TEST_F(ShoppingListTest, RemoveItem) {
    u1.createItem("banana", 10);
    EXPECT_TRUE(u1.removeItem("banana"));
    EXPECT_EQ(u1.getItemsSet().size(), 0);
    EXPECT_FALSE(u1.removeItem("banana"));
}

TEST_F(ShoppingListTest, IncreasingItem) {
    u1.createItem("banana", 10);
    EXPECT_TRUE(u1.increaseItem("banana", 10));
    EXPECT_EQ(u1.getItemsSet().begin()->second, 20);
    EXPECT_FALSE(u1.increaseItem("apple", 10));
}

TEST_F(ShoppingListTest, BuyingItem) {
    u1.createItem("banana", 10);
    EXPECT_TRUE(u1.buyItem("banana", 10));
    EXPECT_EQ(u1.getItemsSet().size(), 0);
    EXPECT_FALSE(u1.buyItem("banana", 10));
}

TEST_F(ShoppingListTest, JoiningShoppingLists) {
    u1.createItem("banana", 10);
    u2.createItem("apple", 20);
    EXPECT_TRUE(u1.join(u2));
    EXPECT_EQ(u1.getItemsSet().size(), 2);
    EXPECT_EQ(u1.getItem("banana").second, 10);
    EXPECT_EQ(u1.getItem("apple").second, 20);

    EXPECT_FALSE(u1.join(u3));
    EXPECT_FALSE(u1.join(u4));
}

TEST_F(ShoppingListTest, JoiningComplex) {
    u1.createItem("banana", 10);
    u2.createItem("banana", 20);
    u1.createItem("apple", 30);
    u1.buyItem("banana");

    EXPECT_TRUE(u1.join(u2));
    EXPECT_EQ(u1.getItemsSet().size(), 2);
    EXPECT_EQ(u1.getItem("banana").second, 29);

    EXPECT_TRUE(u2.join(u1));
    EXPECT_EQ(u2.getItemsSet().size(), 2);
    EXPECT_EQ(u2.getItem("banana").second, 29);

    u2.removeItem("banana");
    u2.removeItem("apple");
    u1.increaseItem("banana", 10);

    EXPECT_TRUE(u1.join(u2));
    EXPECT_EQ(u1.getItemsSet().size(), 1);
    EXPECT_EQ(u1.getItem("banana").second, 19);  // Add wins
    EXPECT_EQ(u1.getItem("apple").second, 0);
}

TEST_F(ShoppingListTest, GetItem) {
    u1.createItem("banana", 10);
    u1.createItem("apple", 20);
    EXPECT_EQ(u1.getItem("banana").second, 10);
    EXPECT_EQ(u1.getItem("apple").second, 20);
    EXPECT_EQ(u1.getItem("orange").second, 0);
}

TEST_F(ShoppingListTest, Contains) {
    u1.createItem("banana", 10);
    EXPECT_TRUE(u1.contains("banana"));
    EXPECT_FALSE(u1.contains("apple"));
}

TEST_F(ShoppingListTest, GetQuantity) {
    u1.createItem("banana", 10);
    EXPECT_EQ(u1.getQuantity("banana"), 10);
    EXPECT_EQ(u1.getQuantity("apple"), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
