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

    ///@brief Завершить лексему и добавить её в таблицу символов
    void CompleteLexeme();

    void AddError(std::string&& err);

    ///@brief Вернуть ошибку под номером idx
    ///@param idx Индекс ошибки
    ///@returns std::nullopt если ошибок нет, либо если нет ошибки под таким индексом, иначе ошибку
    std::optional<std::string> GetError(size_t idx) const;

private:
    std::string currentLexeme_;
    std::unordered_set<std::string> symbolTable_;
    std::stack<std::string> codeStack_;
    std::vector<std::string> errors_;

    // std::string code_;

    // std::vector<??> lexemeStream_;
};

} // namespace compilers
} // namespace tusur
