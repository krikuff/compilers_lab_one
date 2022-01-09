#include <iostream>

#include <error.h>
#include <helpers.h>
#include <pda.h>

using namespace tusur::compilers;

int main()
{
    try
    {
        CompilationContext context;
        PushdownAutomaton<CompilationContext> pda;
        pda.RegisterTransition("Begin", false,
            [](char symbol, MaybeStackItem, CompilationContext context) -> TransitionResult
            {
                MaybeStackItem retStackItem;
                if( std::isspace(symbol) )
                {
                    return std::pair{"Begin", retStackItem};
                }
                else if( helpers::is_alpha_us(symbol) )
                {
                    context.currentSymbol.name.push_back(symbol);
                    return std::make_optional(std::pair{"IdLvalueRest", retStackItem});
                }
                return std::nullopt;
            });
        pda.ProcessText("agagagaga", "Begin", context);
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Critical error!" << std::endl;
    }
    return 0;
}
