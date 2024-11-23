#include <exception>
#include <clap/clap.h>

CLAP_EXPORT
void __cxa_throw()
{
        std::terminate();
}

CLAP_EXPORT
void __cxa_allocate_exception()
{
        std::terminate();
}