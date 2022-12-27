#include "Identifier.h"

static std::atomic<size_t> identifier = NULL;

size_t getNextIdentifier()
{
    return ++identifier;
}
