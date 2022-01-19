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

std::string PdaResultToString(ProcessingResult const& r)
{
    switch(r)
    {
        case ProcessingResult::Success:
            return "Success";
        case ProcessingResult::EndOfTextNotReached:
            return "EndOfTextNotReached";
        case ProcessingResult::StateIsNotFinal:
            return "StateIsNotFinal";
        case ProcessingResult::IncorrectPdaState:
            return "IncorrectPdaState";
    }
}
} // namespace anonymous

using MaybeChar = MaybeStackItem<char>;
using TransitionResultC = TransitionResult<char>;

void RegisterLabOneStates(PushdownAutomaton<Compilation, char>& pda)
{
    namespace sn = state_names;
    using std::pair, std::nullopt;
#define ONLY_STATE(state) pair{state, nullopt}

    pda.RegisterTransition(sn::Begin, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::Begin);
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return ONLY_STATE(sn::IdLvalueRest);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::IdLvalueRest, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( helpers::is_alnum_us(symbol) )
            {
                // context.currentSymbol.name.push_back(symbol);
                return ONLY_STATE(sn::IdLvalueRest);
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::LeftWhitespace);
            }
            else if( symbol == '=' )
            {
                return ONLY_STATE(sn::Q);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::LeftWhitespace, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::LeftWhitespace);
            }
            else if( symbol == '=' )
            {
                return ONLY_STATE(sn::Q);
            }
            return nullopt;
        });
    pda.RegisterTransition(sn::Q, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( symbol == '(' )
            {
                return pair{sn::Q, '('};
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::Q);
            }
            else if( helpers::is_alpha_us(symbol) )
            {
                return ONLY_STATE(sn::Id);
            }
            else if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::NumInt);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Id, true,
        [](char symbol, MaybeChar stackTop, Compilation) -> TransitionResultC
        {
            if( helpers::is_alnum_us(symbol) )
            {
                return ONLY_STATE(sn::Id);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return ONLY_STATE(sn::Q);
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == ')' && stackTop.has_value() && stackTop.value() == '(' )
            {
                return ONLY_STATE(sn::P);
            }
            return nullopt;
        });
    pda.RegisterTransition(sn::P, true,
        [](char symbol, MaybeChar stackTop, Compilation) -> TransitionResultC
        {
            if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == ')' && stackTop.has_value() && stackTop.value() == '(' )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return ONLY_STATE(sn::Q);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::NumInt, true,
        [](char symbol, MaybeChar stackTop, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::NumInt);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return ONLY_STATE(sn::Q);
            }
            else if( symbol == ')' && stackTop.has_value() && stackTop.value() == '(' )
            {
                return ONLY_STATE(sn::P);
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == '.' )
            {
                return ONLY_STATE(sn::Dot);
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return ONLY_STATE(sn::ExpLetter);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Dot, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::NumFrac);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::NumFrac, true,
        [](char symbol, MaybeChar stackTop, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::NumFrac);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return ONLY_STATE(sn::Q);
            }
            else if( symbol == ')' && stackTop.has_value() && stackTop.value() == '(' )
            {
                return ONLY_STATE(sn::P);
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == 'e' || symbol == 'E' )
            {
                return ONLY_STATE(sn::ExpLetter);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpLetter, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::Exp);
            }
            else if( symbol == '+' || symbol == '-' )
            {
                return ONLY_STATE(sn::ExpSign);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::ExpSign, false,
        [](char symbol, MaybeChar, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::Exp);
            }
            return nullopt;
        });

    pda.RegisterTransition(sn::Exp, true,
        [](char symbol, MaybeChar stackTop, Compilation) -> TransitionResultC
        {
            if( std::isdigit(symbol) )
            {
                return ONLY_STATE(sn::Exp);
            }
            else if( std::isspace(symbol) )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == ')' && stackTop.has_value() && stackTop.value() == '(' )
            {
                return ONLY_STATE(sn::P);
            }
            else if( symbol == '*' || symbol == '+' )
            {
                return ONLY_STATE(sn::Q);
            }
            return nullopt;
        });

#undef ONLY_STATE
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
        if( programData.inputFile.is_open() )
        {
            std::getline(programData.inputFile, input);
        }
        else
        {
            std::getline(std::cin, input);
        }

        auto result = pda.ProcessText(input, state_names::Begin, context);
        std::cout << PdaResultToString(result) << std::endl;

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
