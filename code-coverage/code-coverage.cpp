#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

#include <cpp-tree-sitter.h>


extern "C" {
	TSLanguage* tree_sitter_c();
}


int main() {
	// Create a language and parser.
	ts::Language language = tree_sitter_c();
	ts::Parser parser{ language };

	// Parse the provided string into a syntax tree.
	std::string sourcecode = "";
	ts::Tree tree = parser.parseString(sourcecode);

	// Get the root node of the syntax tree. 
	ts::Node root = tree.getRootNode();

	// Get some child nodes.
	ts::Node array = root.getNamedChild(0);
	ts::Node number = array.getNamedChild(0);

	// Check that the nodes have the expected types.
	assert(root.getType() == "document");
	assert(array.getType() == "array");
	assert(number.getType() == "number");

	// Check that the nodes have the expected child counts.
	assert(root.getNumChildren() == 1);
	assert(array.getNumChildren() == 5);
	assert(array.getNumNamedChildren() == 2);
	assert(number.getNumChildren() == 0);

	// Print the syntax tree as an S-expression.
	auto treestring = root.getSExpr();
	printf("Syntax tree: %s\n", treestring.get());

	return 0;
}