#include "gui-client-factory.hh"


 std::shared_ptr < GuiClientFactory> GuiClientFactory::getInstance() {
auto ptr = _instance.lock();
if (ptr)
   return ptr;
ptr = std::make_shared<GuiClientFactory>();
_instance = ptr;
return ptr;
}
