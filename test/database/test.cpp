#include <gtest/gtest.h>

#include "../../src/crdt/shopping_list.h"
#include "../../src/database.h"

class DatabaseTest : public ::testing::Test {
   protected:
    Database db{};
    std::string file_directory;
    std::string initial_files_directory;
    nlohmann::json shopping_list3 = {
        {"id", 3},
        {"name", "Shopping List 3"},
        {"items", {{{"id", 5}, {"name", "Item 5"}, {"quantity", 5}}, {{"id", 6}, {"name", "Item 6"}, {"quantity", 6}}}}};

    void SetUp() override {
        file_directory = getExecutablePath() + "/test/database/test_data";
        initial_files_directory = getExecutablePath() + "/test/database/test_data_default";

        // remove all files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(file_directory)) {
            std::filesystem::remove(entry.path());
        }

        // copy files from initial_files_directory to file_directory
        std::filesystem::copy(initial_files_directory, file_directory, std::filesystem::copy_options::recursive);
        db.load(file_directory);
    }

    std::string getExecutablePath() {
        std::filesystem::path exe_path = std::filesystem::current_path();
        exe_path = std::filesystem::canonical(exe_path);
        return exe_path.string();
    }
};

TEST_F(DatabaseTest, Load) {
    EXPECT_EQ(db.get("shopping_list1")["name"], "Shopping List 1");
    EXPECT_EQ(db.get("shopping_list1")["id"], 1);
    EXPECT_EQ(db.get("shopping_list1")["items"][0]["id"], 1);
    EXPECT_EQ(db.get("shopping_list1")["items"][0]["name"], "Item 1");
    EXPECT_EQ(db.get("shopping_list1")["items"][0]["quantity"], 1);

    EXPECT_EQ(db.get("shopping_list2")["name"], "Shopping List 2");
    EXPECT_EQ(db.get("shopping_list2")["id"], 2);
    EXPECT_EQ(db.get("shopping_list2")["items"][0]["id"], 3);
    EXPECT_EQ(db.get("shopping_list2")["items"][0]["name"], "Item 3");
    EXPECT_EQ(db.get("shopping_list2")["items"][0]["quantity"], 3);
    EXPECT_EQ(db.get("shopping_list2")["items"][1]["id"], 4);
    EXPECT_EQ(db.get("shopping_list2")["items"][1]["name"], "Item 4");
    EXPECT_EQ(db.get("shopping_list2")["items"][1]["quantity"], 4);
}

TEST_F(DatabaseTest, Save) {
    db.set("shopping_list3", shopping_list3);
    db.save(file_directory);

    Database db2{};
    db2.load(file_directory);

    EXPECT_EQ(db2.get("shopping_list3")["name"], "Shopping List 3");
    EXPECT_EQ(db2.get("shopping_list3")["id"], 3);
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["id"], 5);
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["name"], "Item 5");
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["quantity"], 5);
}

TEST_F(DatabaseTest, Del) {
    db.del("shopping_list1");
    db.save(file_directory);

    Database db2{};
    db2.load(file_directory);

    EXPECT_EQ(db2.get("shopping_list1"), "");
}

TEST_F(DatabaseTest, Set) {
    db.set("shopping_list3", shopping_list3);
    db.save(file_directory);

    Database db2{};
    db2.load(file_directory);

    EXPECT_EQ(db2.get("shopping_list3")["name"], "Shopping List 3");
    EXPECT_EQ(db2.get("shopping_list3")["id"], 3);
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["id"], 5);
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["name"], "Item 5");
    EXPECT_EQ(db2.get("shopping_list3")["items"][0]["quantity"], 5);
}

TEST_F(DatabaseTest, Exists) {
    EXPECT_TRUE(db.exists("shopping_list1"));
    EXPECT_FALSE(db.exists("shopping_list3"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}