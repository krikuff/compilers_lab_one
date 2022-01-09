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

struct MaybeStackItem : public std::optional<char>
{
    using std::optional<char>::optional;
    static MaybeStackItem FromTop(std::stack<char>& stack)
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
using TransitionResult = std::optional<std::pair< std::string, MaybeStackItem >>;

// Переход: функция, принимающая считанный символ, верхушку стека и контекст компиляции
template<typename C>
using Transition = std::function< TransitionResult( char, MaybeStackItem, C&) >;

// FIXME: в другой файл все, что не PDA
struct Symbol
{
    std::string name;
};

struct CompilationContext
{
    Symbol currentSymbol;
    std::vector<Symbol> symbolTable_;
};

enum class ProcessingResult
{
    Success = 0,
    StateIsNotFinal,
    EndOfTextNotReached,
    IncorrectPdaState,
};

template<typename C>
class PushdownAutomaton
{
    struct State
    {
        Transition<C> transition;
        bool isFinal;
    };
    using StateMap = std::map<std::string, State>;

public:
    // @brief Регистрация состояния автомата
    // @param from Имя состояния
    // @param isFinal конечное ли состояние TODO: переделать на enum, чтоб понятно было читать вызов
    // @param transition функция перехода
    typename StateMap::iterator RegisterTransition(std::string const& from, bool isFinal, Transition<C>&& transition);

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
    std::stack<char> stack_;
    StateMap states_;
    typename StateMap::iterator currentState_;
};


// Имплементация

template <typename C>
typename PushdownAutomaton<C>::StateMap::iterator
PushdownAutomaton<C>::RegisterTransition(std::string const& from, bool isFinal, Transition<C>&& transition)
{
    return states_.emplace(std::pair{ from, State{std::move(transition), isFinal} }).first;
}

template<typename C>
bool PushdownAutomaton<C>::NextState(char symbol, C& context)
{
    TransitionResult transitionResult = currentState_->second.transition( symbol, MaybeStackItem::FromTop(stack_), context );
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

template <typename C>
ProcessingResult PushdownAutomaton<C>::ProcessText(std::string const& text, std::string const& startingState, C& context)
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
