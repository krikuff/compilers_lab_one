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

std::string InterpretPdaResult(std::string& input, PdaResult res, bool inputIsAtTerminal)
{
    std::string output;
    if( !inputIsAtTerminal )
    {
        output += input + "\n";
    }

    auto [flags, iter, error] = res;
    if( flags == PdaFlags::Success )
    {
        output += "Success";
    }
    else
    {
        const size_t symbolPos = iter - input.begin();

        for( size_t i = 0; i < symbolPos; ++i )
        {
            output.push_back(' ');
        }
        output += "^ " + error + " PDA flags: ";

        if( flags & PdaFlags::StateIsNotFinal )
        {
            output += "StateIsNotFinal ";
        }
        if( flags & PdaFlags::EndOfTextNotReached )
        {
            output += "EndOfTextNotReached ";
        }
        if( flags & PdaFlags::StackIsNotEmpty )
        {
            output += "StackIsNotEmpty ";
        }
    }
    return output;
}

} // namespace anonymous

using StackOfChars = std::stack<char>;

std::pair<bool, std::string> MakeState(std::string const& stateName)
{
    return {true, stateName};
}

std::pair<bool, std::string> MakeError(std::string const& errorText)
{
    return {false, errorText};
}

void RegisterLabOneStates(PushdownAutomaton<Compilation, char>& pda)
{
    namespace sn = state_names;
    using std::pair, std::nullopt;

    pda.RegisterTransition(sn::Begin, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return MakeState(sn::Begin);
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return MakeState(sn::IdLvalueRest);
            }
            return MakeError("Invalid identifier. Has to begin with alphabetic symbol or underscore.");
        });

    pda.RegisterTransition(sn::IdLvalueRest, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return MakeState(sn::IdLvalueRest);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::LeftWhitespace);
            }
            else if( symbol == '=' )
            {
                return MakeState(sn::Q);
            }
            return MakeError("Invalid identifier. Has to consist of alphanumeric symbols or underscore.");
        });

    pda.RegisterTransition(sn::LeftWhitespace, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return MakeState(sn::LeftWhitespace);
            }
            else if( symbol == '=' )
            {
                return MakeState(sn::Q);
            }
            return MakeError("Only assign \"=\" operator is allowed here.");
        });
    pda.RegisterTransition(sn::Q, false,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( symbol == '(' )
            {
                stack.emplace('(');
                return MakeState(sn::Q);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::Q);
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                return MakeState(sn::Id);
            }
            else if( std::isdigit(symbol) )
            {
                return MakeState(sn::NumInt);
            }
            return MakeError("Should be folowed by identifier, number or parentheses.");
        });

    pda.RegisterTransition(sn::Id, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                return MakeState(sn::Id);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return MakeState(sn::Q);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::P);
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return MakeState(sn::P);
            }
            return MakeError("Should be followed by operator or closing parentheses.");
        });
    pda.RegisterTransition(sn::P, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return MakeState(sn::P);
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return MakeState(sn::P);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return MakeState(sn::Q);
            }
            return MakeError("Should be followed by operator or closing parentheses.");
        });

    pda.RegisterTransition(sn::NumInt, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::NumInt);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return MakeState(sn::Q);
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return MakeState(sn::P);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::P);
            }
            else if( symbol == '.' )
            {
                return MakeState(sn::Dot);
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return MakeState(sn::ExpLetter);
            }
            return MakeError("Integer should either be followed by operator or parentheses or become float with E or \".\".");
        });

    pda.RegisterTransition(sn::Dot, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::NumFrac);
            }
            return MakeError("Only decimal part of the number is allowed here.");
        });

    pda.RegisterTransition(sn::NumFrac, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::NumFrac);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return MakeState(sn::Q);
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return MakeState(sn::P);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::P);
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return MakeState(sn::ExpLetter);
            }
            return MakeError("Decimal number should either be followed by operator or parentheses or become scientific with \"e\".");
        });

    pda.RegisterTransition(sn::ExpLetter, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::Exp);
            }
            else if( symbol == '+' || symbol == '-' )
            {
                return MakeState(sn::ExpSign);
            }
            return MakeError("Only signs + and - are allowed here.");
        });

    pda.RegisterTransition(sn::ExpSign, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::Exp);
            }
            return MakeError("Must be followed by number.");
        });

    pda.RegisterTransition(sn::Exp, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return MakeState(sn::Exp);
            }
            else if( std::isspace(symbol) )
            {
                return MakeState(sn::P);
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return MakeState(sn::P);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return MakeState(sn::Q);
            }
            return MakeError("Should be followed by operator or parentheses.");
        });
}

struct ProgramData
{
    std::fstream inputFile;
};

ProgramData ProcessArgs(int argc, char** argv)
{
    ProgramData data;
    for( int i = 1; i < argc; ++i )
    {
        std::string arg(argv[i]);
        // обработка -h и прочих

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

        Compilation context;
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

        auto result = pda.ProcessText(input.cbegin(), input.cend(), state_names::Begin, context);
        std::cout << InterpretPdaResult(input, result, inputIsAtTerminal) << std::endl;

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
