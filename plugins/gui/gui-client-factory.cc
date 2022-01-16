#include "gui-client-factory.hh"
#include "gui-client.hh"

std::shared_ptr<GuiClientFactory> GuiClientFactory::getInstance() {
   auto ptr = _instance.lock();
   if (ptr)
      return ptr;
   ptr.reset(new GuiClientFactory());
   _instance = ptr;
   return ptr;
}

GuiClientFactory::~GuiClientFactory()
{
}
