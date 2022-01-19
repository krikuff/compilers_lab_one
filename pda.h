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

// Результат перехода: следующее состояние либо ничего, если переход невозможен
template<typename I>
using TransitionResult = std::optional< std::string >;

// Переход: функция, принимающая считанный символ, стек и контекст состояний
template<typename C, typename I>
using Transition = std::function< TransitionResult<I>( char, std::stack<I>&, C&) >;

using PdaResult = std::pair<int, std::string::const_iterator>; // TODO: сделать итератор константным, по-хорошему
enum ProcessingResult: int
{
    Success             = 0,
    StateIsNotFinal     = 1 << 0,
    EndOfTextNotReached = 1 << 1,
    StackIsNotEmpty     = 1 << 2,
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
    // @param text Входные данные
    // @param startingState начальное состояние автомата
    // @param context объект контекста состояний
    PdaResult ProcessText(std::string::const_iterator textBegin, std::string::const_iterator textEnd,
                          std::string const& startingState, C& context);

private:
    // Перейти в следующее состояние
    // true если переход возможен, иначе false
    bool NextState(char symbol, C& context);

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
bool PushdownAutomaton<C, I>::NextState(char symbol, C& context)
{
    auto transitionResult = currentState_->second.transition( symbol, stack_, context );
    if( !transitionResult )
    {
        return false;
    }

    auto nextStateName = transitionResult.value();

    currentState_ = states_.find(nextStateName);
    if( currentState_ == states_.end() )
    {
        throw InvalidState();
    }

    return true;
}

template <typename C, typename I>
PdaResult PushdownAutomaton<C, I>::ProcessText(std::string::const_iterator textBegin, std::string::const_iterator textEnd,
                                               std::string const& startingState, C& context)
{
    using enum ProcessingResult;

    currentState_ = states_.find(startingState);
    if(currentState_ == states_.end())
    {
        throw PdaError("Invalid starting state");
    }

    auto currentSymbol = textBegin;
    for(; currentSymbol != textEnd; ++currentSymbol)
    {
        if( !NextState(*currentSymbol, context) )
        {
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
    return std::pair{ret, currentSymbol};
}

} // namespace compilers
} // namespace tusur
