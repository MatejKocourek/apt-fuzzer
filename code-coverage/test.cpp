#include <gtest/gtest.h>
#include "FileInstrument.h"
#include <optional>


// Test Fixture for FileInstrument tests
class FileInstrumentTest : public ::testing::Test {
protected:
    std::optional<FileInstrument> fileInstrument;

    void SetUp() override {
        fileInstrument.emplace("int main() { return 0; }", "test.cpp", 1);
    }

    void TearDown() override {
        fileInstrument.reset();
    }
};

// Test Fixture for FileInstrument tests
class FileInstrumentEmpty : public ::testing::Test {
protected:
    std::optional<FileInstrument> fileInstrument;

    void SetUp() override {
        fileInstrument.emplace(" ", "test.cpp", 1);
    }

    void TearDown() override {
        fileInstrument.reset();
    }
};

// Test constructor and parseSource
TEST_F(FileInstrumentTest, ConstructorAndParseSource) {
    EXPECT_EQ(fileInstrument->filename, "test.cpp");
    EXPECT_EQ(fileInstrument->fileId, 1);
    EXPECT_EQ(fileInstrument->sourcecode, "int main() { return 0; }");
}

// Test instrumentLine function
TEST_F(FileInstrumentEmpty, InstrumentLine) {
    fileInstrument->instrumentLine(10, 5);
    ASSERT_EQ(fileInstrument->instrumentations.size(), 1);
    EXPECT_EQ(fileInstrument->instrumentations[0].first, 10);
    EXPECT_EQ(fileInstrument->instrumentations[0].second, 6);
    EXPECT_EQ(fileInstrument->lastInstrumentedLine, 5);

    // Test that the same line isn't instrumented again
    fileInstrument->instrumentLine(20, 5);
    EXPECT_EQ(fileInstrument->instrumentations.size(), 1); // No new instrumentation
}

// Test instrumentChildren with real Tree-sitter node
TEST_F(FileInstrumentEmpty, InstrumentChildren) {
    std::string code = "{if (true) return 1; else return 0;}";

    auto tree = ts::Parser{ tree_sitter_c() }.parseString(std::move(code));
    ts::Node rootNode = tree.getRootNode().getChild(0);

    // Traverse the parsed source code and instrument children nodes
    fileInstrument->instrumentRecursive(rootNode);

    EXPECT_GT(fileInstrument->instrumentations.size(), 0); // Expect some instrumentation to be added
}

// Test instrumentPossibleOneLiner with real Tree-sitter node
TEST_F(FileInstrumentEmpty, InstrumentPossibleOneLiner) {
    std::string code = "if (true) return 1;";

    auto tree = ts::Parser{ tree_sitter_c() }.parseString(std::move(code));
    ts::Node rootNode = tree.getRootNode();

    // Assuming the first child of the root node is the if-statement
    ts::Node ifNode = rootNode.getChild(0).getChild(2);

    fileInstrument->instrumentPossibleOneLiner(ifNode);

    EXPECT_EQ(fileInstrument->instrumentationsStr.size(), 2);  // One-liner should be surrounded by {}
    EXPECT_EQ(fileInstrument->instrumentationsStr[0].first, 10);
    EXPECT_EQ(fileInstrument->instrumentationsStr[0].second, "{");
    EXPECT_EQ(fileInstrument->instrumentationsStr[1].first, 19);
    EXPECT_EQ(fileInstrument->instrumentationsStr[1].second, "}");
}

// Test instrumentCompoundStatement with real Tree-sitter node
TEST_F(FileInstrumentEmpty, InstrumentCompoundStatement) {
    std::string code = "{ int x = 0;\n//comment\nif (x)\nx++; }";

    auto tree = ts::Parser{ tree_sitter_c() }.parseString(std::move(code));
    ts::Node rootNode = tree.getRootNode();

    // Assuming the first child of the root node is a compound statement
    ts::Node compoundNode = rootNode.getChild(0);

    fileInstrument->instrumentRecursive(compoundNode);

    EXPECT_EQ(fileInstrument->instrumentations.size(), 3); // Expect three instrumented statements (declaration and if-statement)
}

// Test instrument function
TEST_F(FileInstrumentTest, Instrument) {
    std::stringstream output;
    fileInstrument->instrument(output);

    EXPECT_EQ(output.str(), "int main() {atexit(_GenerateLcov); ++_F1[0];return 0; }");
}

// Test instrumentHeaderExtern function
TEST(InstrumentHeaderTest, InstrumentHeaderExtern) {
    FileInstrument file("int test() { return 0; }", "test.cpp", 1);
    std::stringstream output;

    instrumentHeaderExtern(output, file);

    EXPECT_EQ(output.str(), "extern unsigned long long _F1[];\n");
}

// Test instrumentHeaderMain function
TEST(InstrumentHeaderTest, InstrumentHeaderMain) {
    std::vector<FileInstrument> allFiles = {
        FileInstrument("int main() { return 0; }", "file0.cpp", 0),
        FileInstrument("void foo() { }", "file1.cpp", 1)
    };

    std::stringstream output;
    instrumentHeaderMain(output, allFiles);

    EXPECT_FALSE(output.str().empty());
}