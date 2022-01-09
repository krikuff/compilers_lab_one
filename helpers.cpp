#include <helpers.h>

#include <cctype>

namespace tusur
{
namespace compilers
{
namespace helpers
{

bool is_alpha_us(char c)
{
    return std::isalpha(c) || c == '_';
}

bool is_alnum_us(char c)
{
    return std::isalnum(c) || c == '_';
}

} // namespace helpers
} // namespace compilers
} // namespace tusur
