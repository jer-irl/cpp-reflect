#pragma once

#include <clang/Frontend/ASTUnit.h>

namespace CppReflect {

void generateRegistrationFile(clang::ASTUnit& astUnit, const std::string& outPath, const std::string& mainFilePath);

} // namespace CppReflect
