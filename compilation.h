#pragma once

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace tusur
{
namespace compilers
{

///@brief Тип лексемы
///
/// У операций явно указан приоритет, так что их можно сравнивать как int
enum LexemeType
{
    OpeningParentheses,
    ClosingParentheses,
    Assign         = 10,
    PlusSign       = 11,
    MultipliesSign = 12,
    IntegerNumber,
    FloatingPointNumber,
    Identifier,
};

struct Compilation
{
public:
    void PushToLexeme(char symbol);

    ///@brief Завершить лексему и добавить её в таблицу символов
    void CompleteLexeme(LexemeType type);

    void GenerateRemainingCode(); // обработать оставшееся на стеке

    void AddError(std::string&& err);

    std::string GetCode() const;

    ///@brief Вернуть ошибку под номером idx
    ///@param idx Индекс ошибки
    ///@returns std::nullopt если ошибок нет, либо если нет ошибки под таким индексом, иначе ошибку
    std::optional<std::string> GetError(size_t idx) const;

private:
    // Сгенерировать код на стеках без проверок стеков
    void GenerateCodeOnce();

private:
    std::string currentLexeme_;
    std::unordered_map<std::string, LexemeType> symbolTable_;
    std::stack<std::string> codeStack_;
    std::stack<LexemeType> opStack_; // FIXME: как-то неправильно тут держать тип лексемы, но работает пока
    std::vector<std::string> errors_;

    // std::string code_;

    // std::vector<??> lexemeStream_;
};

} // namespace compilers
} // namespace tusur
