#pragma once

#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

namespace tusur
{
namespace compilers
{

struct Compilation
{
public:
    void PushToLexeme(char symbol);
    void CompleteLexeme();

private:
    std::string currentLexeme_;
    std::unordered_set<std::string> symbolTable_;
    std::stack<std::string> codeStack_;

    std::string code_;

    // std::vector<??> lexemeStream_;
};

} // namespace compilers
} // namespace tusur
