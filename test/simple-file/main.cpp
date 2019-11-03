#include <CppReflect/CppReflect.hpp>

int main() {
    const clang::ASTContext& mainASTContext = CppReflect::astForTranslationUnit("main.cpp");
    return 0;
}
