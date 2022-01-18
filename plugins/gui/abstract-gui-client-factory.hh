#pragma once

#include <memory>
#include <string>
#include <vector>

namespace clap {

   class AbstractGui;
   class AbstractGuiListener;
   class AbstractGuiClientFactory {
   public:
      virtual ~AbstractGuiClientFactory() = 0;

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) = 0;
   };

} // namespace clap