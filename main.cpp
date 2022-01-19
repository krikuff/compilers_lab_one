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

std::string PdaResultToString(std::string& input, PdaResult res, bool inputIsAtTerminal)
{
    std::string error;
    if( !inputIsAtTerminal )
    {
        error += input + "\n";
    }

    auto [flags, iter] = res;
    if( flags == PdaFlags::Success )
    {
        error += "Success";
    }
    else
    {
        const size_t symbolPos = iter - input.begin();

        for( size_t i = 0; i < symbolPos - 1; ++i )
        {
            error.push_back(' ');
        }
        error += "^ ";

        if( flags & PdaFlags::StateIsNotFinal )
        {
            error += "StateIsNotFinal ";
        }
        if( flags & PdaFlags::EndOfTextNotReached )
        {
            error += "EndOfTextNotReached ";
        }
        if( flags & PdaFlags::StackIsNotEmpty )
        {
            error += "StackIsNotEmpty ";
        }
    }
    return error;
}

} // namespace anonymous

using StackOfChars = std::stack<char>;

void RegisterLabOneStates(PushdownAutomaton<Compilation, char>& pda)
{
    namespace sn = state_names;
    using std::pair, std::nullopt;

    pda.RegisterTransition(sn::Begin, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::Begin;
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return sn::IdLvalueRest;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::IdLvalueRest, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return sn::IdLvalueRest;
            }
            else if( std::isspace(symbol) )
            {
                return sn::LeftWhitespace;
            }
            else if( symbol == '=' )
            {
                return sn::Q;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::LeftWhitespace, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::LeftWhitespace;
            }
            else if( symbol == '=' )
            {
                return sn::Q;
            }
            return nullopt;
        });
    pda.RegisterTransition(sn::Q, false,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( symbol == '(' )
            {
                stack.emplace('(');
                return sn::Q;
            }
            else if( std::isspace(symbol) )
            {
                return sn::Q;
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                return sn::Id;
            }
            else if( std::isdigit(symbol) )
            {
                return sn::NumInt;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Id, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( helpers::is_alnum_us(symbol) )
            {
                return sn::Id;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return sn::Q;
            }
            else if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return sn::P;
            }
            return nullopt;
        });
    pda.RegisterTransition(sn::P, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return sn::P;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return sn::Q;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::NumInt, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::NumInt;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return sn::Q;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return sn::P;
            }
            else if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == '.' )
            {
                return sn::Dot;
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return sn::ExpLetter;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Dot, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::NumFrac;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::NumFrac, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::NumFrac;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return sn::Q;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return sn::P;
            }
            else if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return sn::ExpLetter;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpLetter, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::Exp;
            }
            else if( symbol == '+' || symbol == '-' )
            {
                return sn::ExpSign;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpSign, false,
        [](char symbol, StackOfChars&, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::Exp;
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Exp, true,
        [](char symbol, StackOfChars& stack, Compilation&) -> TransitionResult
        {
            if( std::isdigit(symbol) )
            {
                return sn::Exp;
            }
            else if( std::isspace(symbol) )
            {
                return sn::P;
            }
            else if( symbol == ')' && !stack.empty() && stack.top() == '(' )
            {
                stack.pop();
                return sn::P;
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return sn::Q;
            }
            return nullopt;
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
        std::cout << PdaResultToString(input, result, inputIsAtTerminal) << std::endl;

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
