#include <compilation.h>

#include <errors.h>

namespace tusur
{
namespace compilers
{

namespace
{

std::string OperatoinCode(LexemeType operation, std::string const& lhs, std::string const& rhs, int Register)
{
    // TODO: код формируется прямо как в презентации, не очень интуитивно. Подумать над способами лучше
    std::string code;
    const auto RegisterName = std::to_string(Register);
    switch( operation )
    {
        case Assign:
            code += "LOAD " + rhs
                 + "\nSTORE " + lhs;
            break;
        case PlusSign:
            code += rhs
                 + "\nSTORE $" + RegisterName
                 + "\nLOAD " + lhs
                 + "\nADD $" + RegisterName;
            break;
        case MultipliesSign:
            code += rhs
                 + "\nSTORE $" + RegisterName
                 + "\nLOAD " + lhs
                 + "\nMPY $" + RegisterName;
            break;
        default:
            throw CompilationError("Unknown operation: " + LexemeTypeToString(operation));
            break;
    }
    return code;
}

} // namespace anonymous

std::string LexemeTypeToString(LexemeType type)
{
    using enum LexemeType;
    switch( type )
    {
        case Assign:
            return "Assing";
        case PlusSign:
            return "PlusSign";
        case MultipliesSign:
            return "MultipliesSign";
        case OpeningParentheses:
            return "OpeningParentheses";
        case ClosingParentheses:
            return "ClosingParentheses";
        case IntegerNumber:
            return "IntegerNumber";
        case FloatingPointNumber:
            return "FloatingPointNumber";
        case Identifier:
            return "Identifier";
        default:
            return "Not lexeme type(" + std::to_string(type) + ")";
    }
}

void Compilation::PushToLexeme(char symbol)
{
    currentLexeme_.push_back(symbol);
}

void Compilation::CompleteLexeme(LexemeType type)
{
    auto [lexeme, isInserted] = symbolTable_.emplace( std::move(currentLexeme_), type );
    currentLexeme_.clear(); // надо ли?

    switch (type)
    {
        case Identifier:
        case IntegerNumber:
        case FloatingPointNumber:
        {
            codeStack_.push({ lexeme->first, std::bitset<MAX_REGISTER_COUNT>() });
            break;
        }

        case Assign:
        case PlusSign:
        case MultipliesSign:
        {
            if( opStack_.empty() )
            {
                opStack_.push(type);
                break;
            }

            // TODO: можно ли засунуть сюда скобки?
            while( type <= opStack_.top() ) // нестрогий знак т.к. операции с равным приоритетом выполняются слева направо
            {
                if( codeStack_.size() < 2 )
                {
                    throw CompilationError("Not enough operands on stack!");
                }
                GenerateCodeOnce();
            }

            opStack_.push(type);
            break;
        }
        case OpeningParentheses:
        {
            opStack_.push(type);
            break;
        }
        case ClosingParentheses:
        {
            while( opStack_.top() != OpeningParentheses )
            {
                GenerateCodeOnce();
            }
            opStack_.pop();
            break;
        }
        default:
        {
            throw CompilationError("Unknown lexeme type");
        }
    }
}

void Compilation::GenerateCodeOnce()
{
    auto opType = opStack_.top();
    opStack_.pop();
    auto [rhs, rhsRegisters] = codeStack_.top();
    codeStack_.pop();
    auto [lhs, lhsRegisters] = codeStack_.top();
    codeStack_.pop();

    // TODO: выбор регистра можно оптимизировать, если увидеть, что те регистры, что были использованы в
    // rhs, всегда можно переиспользовать

    auto usedRegisters = lhsRegisters | rhsRegisters;
    int availableRegister = usedRegisters.count();
    usedRegisters |= 1 << availableRegister;
    codeStack_.push({ OperatoinCode(opType, lhs, rhs, availableRegister), usedRegisters });
}

void Compilation::GenerateRemainingCode()
{
    while( !opStack_.empty() )
    {
        GenerateCodeOnce();
    }
}

std::string Compilation::GetCode() const
{
    return codeStack_.top().code;
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
