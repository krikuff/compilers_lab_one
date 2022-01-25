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

// Результат перехода. Ничего, если переход невозможен, либо имя следующего состояния.
using TransitionResult = std::optional<std::string>;

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
    using Finalizer = std::function<void( char, std::string const&, std::stack<I>&, C& )>; // TODO: некрасиво дублируются параметры тут и в Transition

public:
    ///@brief Регистрация состояния автомата
    ///@param from Имя состояния
    ///@param isFinal конечное ли состояние TODO: переделать на enum, чтоб понятно было читать вызов
    ///@param transition функция перехода
    typename StateMap::iterator RegisterTransition(std::string const& from, bool isFinal, Transition<C, I>&& transition);

    void SetFinalizer(Finalizer&& finalizer);

    // @brief Обработать текст автоматом
    // @param textBegin Итератор начала строки входных данных // TODO: неконсистентно как-то. Перейти бы везде на ranges
    // @param textEnd Итератор конца строки входных данных
    // @param startingState начальное состояние автомата
    // @param context объект контекста состояний
    PdaResult ProcessText(std::string::const_iterator textBegin, std::string::const_iterator textEnd,
                          std::string const& startingState, C& context);

private:
    // Перейти в следующее состояние
    // true, если переход произошел
    bool NextState(char symbol, C& context);

private:
    std::stack<I> stack_;
    StateMap states_;
    Finalizer finalizer_;
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
void PushdownAutomaton<C, I>::SetFinalizer(Finalizer&& finalizer)
{
    finalizer_ = std::move( finalizer );
}

template<typename C, typename I>
bool PushdownAutomaton<C, I>::NextState(char symbol, C& context)
{
    auto nextStateName = currentState_->second.transition( symbol, stack_, context );
    if( !nextStateName )
    {
        return false;
    }

    currentState_ = states_.find( nextStateName.value() );
    if( currentState_ == states_.end() )
    {
        throw InvalidState();
    }

    return true;
}

template <typename C, typename I>
PdaResult PushdownAutomaton<C, I>::ProcessText(std::string::const_iterator textBegin,
                                               std::string::const_iterator textEnd,
                                               std::string const& startingState, C& context)
{
    using enum PdaFlags;

    currentState_ = states_.find( startingState );
    if( currentState_ == states_.end() )
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
    else
    {
        finalizer_(*(textEnd - 1), currentState_->first, stack_, context);
    }
    if( !currentState_->second.isFinal )
    {
        ret |= StateIsNotFinal;
    }
    if( !stack_.empty() )
    {
        ret |= StackIsNotEmpty;
    }
    return {ret, currentSymbol};
}

} // namespace compilers
} // namespace tusur
