#include <algorithm>
#include <iostream>
#include <fstream>

#include <compilation.h>
#include <error.h>
#include <helpers.h>
#include <pda.h>

using namespace tusur::compilers;

namespace
{
namespace state_names
{
    const std::string Begin          = "Begin";
    const std::string IdLvalueRest   = "IdLvalueRest";
    const std::string LeftWhitespace = "LeftWhitespace";
    const std::string Q              = "Q";
    const std::string Id             = "Id";
    const std::string P              = "P";
    const std::string NumInt         = "NumInt";
    const std::string Dot            = "Dot";
    const std::string NumFrac        = "NumFrac";
    const std::string ExpLetter      = "ExpLetter";
    const std::string ExpSign        = "ExpSign";
    const std::string Exp            = "Exp";
} // namespace state_names

std::string PdaFlagsToString(int flags)
{
    if( flags == PdaFlags::Success )
    {
        return "Success";
    }

    std::string ret;
    if( flags & PdaFlags::StateIsNotFinal )
    {
        ret += "StateIsNotFinal ";
    }
    if( flags & PdaFlags::EndOfTextNotReached )
    {
        ret += "EndOfTextNotReached ";
    }
    if( flags & PdaFlags::StackIsNotEmpty )
    {
        ret += "StackIsNotEmpty ";
    }

    return ret;
}

std::string InterpretPdaResult(std::string& input, PdaResult res, std::optional<std::string> error, bool inputIsAtTerminal)
{
    std::string output;
    if( !inputIsAtTerminal )
    {
        output += input + "\n";
    }

    auto [flags, iter] = res;
    if( flags == PdaFlags::Success )
    {
        output += "Correct";
        return output;
    }

    const size_t symbolPos = iter - input.begin();
    for( size_t i = 0; i < symbolPos; ++i )
    {
        output.push_back(' ');
    }

    const std::string errorMsg = error.has_value()
                                 ? " " + error.value()
                                 : "";
    output += "^" + errorMsg + " PDA flags: " + PdaFlagsToString(flags);

    return output;
}

} // namespace anonymous

using StackOfChars = std::stack<char>;

void RegisterLabOneStates(PushdownAutomaton<Compilation, char>& pda)
{
    namespace sn = state_names;
    using std::pair, std::nullopt;

    pda.RegisterTransition(sn::Begin, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::Begin;
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::IdLvalueRest;
            }
            compilation.AddError("Invalid identifier. Has to begin with alphabetic symbol or underscore.");
            return nullopt;
        });

    pda.RegisterTransition(sn::IdLvalueRest, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::IdLvalueRest;
            }
            else if( std::isspace(symbol) )
            {
                compilation.CompleteLexeme(LexemeType::Identifier);
                return sn::LeftWhitespace;
            }
            else if( symbol == '=' )
            {
                compilation.CompleteLexeme(LexemeType::Identifier);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::Assign);
                return sn::Q;
            }
            compilation.AddError("Invalid identifier. Has to consist of alphanumeric symbols or underscore.");
            return nullopt;
        });

    pda.RegisterTransition(sn::LeftWhitespace, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::LeftWhitespace;
            }
            else if( symbol == '=' )
            {
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::Assign);
                return sn::Q;
            }
            compilation.AddError("Only assign \"=\" operator is allowed here.");
            return nullopt;
        });
    pda.RegisterTransition(sn::Q, false,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( symbol == '(' )
            {
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::OpeningParentheses);

                stack.emplace('(');
                return sn::Q;
            }
            else if( std::isspace(symbol) )
            {
                return sn::Q;
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::Id;
            }
            else if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::NumInt;
            }
            compilation.AddError("Should be an identifier, a number or (.");
            return nullopt;
        });

    pda.RegisterTransition(sn::Id, true,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::Id;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                compilation.CompleteLexeme(LexemeType::Identifier);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme( (symbol == '*')? LexemeType::MultipliesSign : LexemeType::PlusSign );
                return sn::Q;
            }
            else if( std::isspace(symbol) )
            {
                compilation.CompleteLexeme(LexemeType::Identifier);
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                compilation.CompleteLexeme(LexemeType::Identifier);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::ClosingParentheses);

                stack.pop();
                return sn::P;
            }
            compilation.AddError("Should be an operator or ).");
            return nullopt;
        });
    pda.RegisterTransition(sn::P, true,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::ClosingParentheses);

                stack.pop();
                return sn::P;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme( (symbol == '*')? LexemeType::MultipliesSign : LexemeType::PlusSign );

                return sn::Q;
            }
            compilation.AddError("Should be an operator or ).");
            return nullopt;
        });

    pda.RegisterTransition(sn::NumInt, true,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::NumInt;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                compilation.CompleteLexeme(LexemeType::IntegerNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme( (symbol == '*')? LexemeType::MultipliesSign : LexemeType::PlusSign );
                return sn::Q;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                compilation.CompleteLexeme(LexemeType::IntegerNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::ClosingParentheses);

                stack.pop();
                return sn::P;
            }
            else if( std::isspace(symbol) )
            {
                compilation.CompleteLexeme(LexemeType::IntegerNumber);
                return sn::P;
            }
            else if( symbol == '.' )
            {
                compilation.PushToLexeme(symbol);
                return sn::Dot;
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                compilation.PushToLexeme(symbol);
                return sn::ExpLetter;
            }
            compilation.AddError("Integer should either be followed by an operator or ) or become a float with E or \".\".");
            return nullopt;
        });

    pda.RegisterTransition(sn::Dot, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::NumFrac;
            }
            compilation.AddError("Only decimal part of the number is allowed here.");
            return nullopt;
        });

    pda.RegisterTransition(sn::NumFrac, true,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::NumFrac;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme( (symbol == '*')? LexemeType::MultipliesSign : LexemeType::PlusSign );

                return sn::Q;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::ClosingParentheses);

                stack.pop();
                return sn::P;
            }
            else if( std::isspace(symbol) )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                return sn::P;
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                compilation.PushToLexeme(symbol);
                return sn::ExpLetter;
            }
            compilation.AddError("Decimal number should either be an operator or ) or become a scientific with \"e\".");
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpLetter, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::Exp;
            }
            else if( symbol == '+' || symbol == '-' )
            {
                compilation.PushToLexeme(symbol);
                return sn::ExpSign;
            }
            compilation.AddError("Only signs + and - are allowed here.");
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpSign, false,
        [](char symbol, StackOfChars&, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::Exp;
            }
            compilation.AddError("Must be a number.");
            return nullopt;
        });

    pda.RegisterTransition(sn::Exp, true,
        [](char symbol, StackOfChars& stack, Compilation& compilation) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                compilation.PushToLexeme(symbol);
                return sn::Exp;
            }
            else if( std::isspace(symbol) )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme(LexemeType::ClosingParentheses);

                stack.pop();
                return sn::P;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                compilation.CompleteLexeme(LexemeType::FloatingPointNumber);
                compilation.PushToLexeme(symbol);
                compilation.CompleteLexeme( (symbol == '*')? LexemeType::MultipliesSign : LexemeType::PlusSign );

                return sn::Q;
            }
            compilation.AddError("Should be an operator or ).");
            return nullopt;
        });

    pda.SetFinalizer( [](char, std::string const& prevState, StackOfChars&, Compilation& compilation)
    {
        // TODO: очень грязный код, отрефакторить
        auto lexemeType = LexemeType::Assign;
        if( prevState == sn::Id )
        {
            lexemeType = LexemeType::Identifier;
        }
        else if( prevState == sn::NumInt )
        {
            lexemeType = LexemeType::IntegerNumber;
        }
        else if( prevState == sn::NumFrac || prevState == sn::Exp )
        {
            lexemeType = LexemeType::FloatingPointNumber;
        } // еще из конечных состояний есть P, но перед ним лексемы всегда коммитятся в compilation

        if( lexemeType != LexemeType::Assign )
        {
            compilation.CompleteLexeme(lexemeType);
        }
    });
}

struct ProgramData
{
    std::fstream inputFile;
    // TODO: добавить файл вывода с опцией -o
};

ProgramData ProcessArgs(int argc, char** argv)
{
    ProgramData data;
    for( int i = 1; i < argc; ++i )
    {
        std::string arg(argv[i]);
        // TODO: обработка -h и прочих

        if( !data.inputFile.is_open() )
        {
            data.inputFile.open(arg);
            if( !data.inputFile.good() )
            {
                throw std::runtime_error("Couldn't open file");
            }
        }
    }

    return data;
}

int main(int argc, char** argv)
{
    try
    {
        auto programData = ProcessArgs(argc, argv);

        Compilation compilation;
        PushdownAutomaton<Compilation, char> pda;
        RegisterLabOneStates(pda);

        std::string input;
        bool inputIsAtTerminal = false;
        if( programData.inputFile.is_open() )
        {
            std::getline(programData.inputFile, input);
        }
        else
        {
            std::getline(std::cin, input);
            inputIsAtTerminal = true;
        }

        auto result = pda.ProcessText(input.cbegin(), input.cend(), state_names::Begin, compilation);

        if( result.flags == Success ) // TODO: очень грязный наколеночный код, переписать чисто
        {
            compilation.GenerateRemainingCode();
        }

        std::cout << InterpretPdaResult(input, result, compilation.GetError(0), inputIsAtTerminal) << std::endl;

        if( result.flags == Success )
        {
            std::cout << "\nCode:\n" << compilation.GetCode() << std::endl;
            auto symbolTable = compilation.GetSymbolTable();
            std::cout << "\nSymbol table:\n";
            std::for_each(symbolTable.begin(), symbolTable.end(), [](auto& symbol)
            {
                std::cout << "\t" << LexemeTypeToString(symbol.second) << " " << symbol.first << "\n";
            });
        }

        return 0;
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch(...)
    {
        std::cerr << "Critical error!" << std::endl;
        return -2;
    }
}
