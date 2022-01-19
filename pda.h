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

template<typename I>
struct MaybeStackItem : public std::optional<I>
{
    using std::optional<I>::optional;
    static MaybeStackItem FromTop(std::stack<I>& stack)
    {
        if( stack.empty() )
        {
            return std::nullopt;
        }

        auto ret = stack.top();
        stack.pop();
        return ret;
    }
};

// Результат перехода: следующее состояние и то, что надо положить на стек, либо ничего если переход невозможен
template<typename I>
using TransitionResult = std::optional<std::pair< std::string, MaybeStackItem<I> >>;

// Переход: функция, принимающая считанный символ, верхушку стека и контекст компиляции
template<typename C, typename I>
using Transition = std::function< TransitionResult<I>( char, MaybeStackItem<I>, C&) >;

enum class ProcessingResult
{
    Success = 0,
    StateIsNotFinal,
    EndOfTextNotReached,
    IncorrectPdaState,
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
    ProcessingResult ProcessText(std::string const& text, std::string const& startingState, C& context);

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
    auto transitionResult = currentState_->second.transition( symbol, MaybeStackItem<I>::FromTop(stack_), context );
    if( !transitionResult )
    {
        return false;
    }

    auto[nextStateName, stackTop] = transitionResult.value();

    currentState_ = states_.find(nextStateName);
    if( currentState_ == states_.end() )
    {
        throw InvalidState();
    }

    if( stackTop )
    {
        stack_.push( stackTop.value() );
    }
    return true;
}

template <typename C, typename I>
ProcessingResult PushdownAutomaton<C, I>::ProcessText(std::string const& text, std::string const& startingState, C& context)
{
    using enum ProcessingResult;

    currentState_ = states_.find(startingState);
    if(currentState_ == states_.end())
    {
        throw PdaError("Invalid starting state");
    }

    auto currentSymbol = text.begin();
    for(; currentSymbol != text.end(); ++currentSymbol)
    {
        if( !NextState(*currentSymbol, context) )
        {
            break;
        }
    }

    ProcessingResult ret = Success;
    bool endReached = ( currentSymbol == text.end() );
    bool stateIsFinal = currentState_->second.isFinal;
    if (!endReached && !stateIsFinal)
    {
        ret = IncorrectPdaState;
    }
    else if( !endReached )
    {
        ret = EndOfTextNotReached;
    }
    else if( !stateIsFinal )
    {
        ret = StateIsNotFinal;
    }
    return ret;
}

} // namespace compilers
} // namespace tusur
