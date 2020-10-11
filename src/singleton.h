//

#pragma once

#include <vector>
#include <list>
#include <memory>

template<typename t> class singleton
{
public:
    static t &instance() { static t i; return i; }

protected:
    singleton() {}
    
public:
    singleton(singleton const&) = delete;
    void operator=(singleton const&) = delete;
};
