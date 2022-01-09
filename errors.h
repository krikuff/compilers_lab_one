#pragma once

#include <stdexcept>

namespace tusur
{
namespace compilers
{

struct PdaError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct InvalidState : public PdaError
{
    using PdaError::PdaError; // TODO: возможно, это надо убрать
    InvalidState() : PdaError("Attempt to transit to invalid state") {}
};

} // namespace compilers
} // namespace tusur
