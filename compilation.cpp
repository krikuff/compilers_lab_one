#include <compilation.h>

namespace tusur
{
namespace compilers
{

void Compilation::PushToLexeme(char symbol)
{
    currentLexeme_.push_back(symbol);
}

void Compilation::CompleteLexeme()
{
    symbolTable_.emplace(std::move( currentLexeme_ ));
    currentLexeme_.clear();

    //TODO: генерировать код прямо тут
}

} // namespace compilers
} // namespace tusur
