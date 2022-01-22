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

void Compilation::AddError(std::string&& err)
{
    errors_.emplace_back(std::move( err ));
}

std::optional<std::string> Compilation::GetError(size_t idx) const
{
    if(errors_.empty() || errors_.size() >= idx)
    {
        return std::nullopt;
    }
    return errors_[idx];
}

} // namespace compilers
} // namespace tusur
