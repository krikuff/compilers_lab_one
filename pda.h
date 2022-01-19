#pragma once

#include <functional>
#include <map>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include <errors.h>

namespace tusur
{
namespace compilers
{

// Результат перехода. В результате перехода можно получить либо имя следующего состояния, либо текст ошибки.
// Булевый флаг true, если строка -- имя состояния.
using TransitionResult = std::pair<bool, std::string>;

// Функция перехода. Принимает считанный символ, стек и контекст состояний.
template<typename C, typename I>
using Transition = std::function< TransitionResult( char, std::stack<I>&, C&) >;

enum PdaFlags: int
{
    Success             = 0,
    StateIsNotFinal     = 1 << 0,
    EndOfTextNotReached = 1 << 1,
    StackIsNotEmpty     = 1 << 2,
};

struct PdaResult
{
    int flags;
    std::string::const_iterator errorPosition;
    std::string errorText;
};

template<typename C, typename I>
class PushdownAutomaton
{
    struct State
    {
        Transition<C, I> transition;
        bool isFinal;
    };
    using StateMap = std::map<std::string, State>;

public:
    // @brief Регистрация состояния автомата
    // @param from Имя состояния
    // @param isFinal конечное ли состояние TODO: переделать на enum, чтоб понятно было читать вызов
    // @param transition функция перехода
    typename StateMap::iterator RegisterTransition(std::string const& from, bool isFinal, Transition<C, I>&& transition);

    // @brief Обработать текст автоматом
    // @param textBegin Итератор начала строки входных данных // TODO: неконсистентно как-то. Перейти бы везде на ranges
    // @param textEnd Итератор конца строки входных данных
    // @param startingState начальное состояние автомата
    // @param context объект контекста состояний
    PdaResult ProcessText(std::string::const_iterator textBegin, std::string::const_iterator textEnd,
                          std::string const& startingState, C& context);

private:
    // Перейти в следующее состояние
    // Возвращает строку ошибки, если переход невозможен, иначе ничего
    std::optional<std::string> NextState(char symbol, C& context);

private:
    std::stack<I> stack_;
    StateMap states_;
    typename StateMap::iterator currentState_;
};


// Имплементация

template <typename C, typename I>
typename PushdownAutomaton<C, I>::StateMap::iterator
PushdownAutomaton<C, I>::RegisterTransition(std::string const& from, bool isFinal, Transition<C, I>&& transition)
{
    return states_.emplace(std::pair{ from, State{std::move(transition), isFinal} }).first;
}

template<typename C, typename I>
std::optional<std::string> PushdownAutomaton<C, I>::NextState(char symbol, C& context)
{
    auto [transitionSuccessfull, stateOrErr] = currentState_->second.transition( symbol, stack_, context );
    if( !transitionSuccessfull )
    {
        return stateOrErr;
    }

    currentState_ = states_.find(stateOrErr);
    if( currentState_ == states_.end() )
    {
        throw InvalidState();
    }

    return std::nullopt;
}

template <typename C, typename I>
PdaResult PushdownAutomaton<C, I>::ProcessText(std::string::const_iterator textBegin,
                                               std::string::const_iterator textEnd,
                                               std::string const& startingState, C& context)
{
    using enum PdaFlags;

    currentState_ = states_.find(startingState);
    if(currentState_ == states_.end())
    {
        throw PdaError("Invalid starting state");
    }

    std::string error;
    auto currentSymbol = textBegin;
    for(; currentSymbol != textEnd; ++currentSymbol)
    {
        if( auto errorText = NextState(*currentSymbol, context);
            errorText.has_value() )
        {
            error = errorText.value();
            break;
        }
    }

    int ret = Success;
    if( currentSymbol != textEnd )
    {
        ret |= EndOfTextNotReached;
    }
    if( !currentState_->second.isFinal )
    {
        ret |= StateIsNotFinal;
    }
    if( !stack_.empty() )
    {
        ret |= StackIsNotEmpty;
    }
    return {ret, currentSymbol, error};
}

} // namespace compilers
} // namespace tusur
