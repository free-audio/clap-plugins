#pragma once

#include <memory>
#include <string>
#include <vector>

namespace clap {

   class AbstractGui;
   class AbstractGuiListener;
   class AbstractGuiFactory {
   public:
      virtual ~AbstractGuiFactory();

      virtual std::shared_ptr<AbstractGui>
      createGuiClient(AbstractGuiListener &listener,
                      const std::vector<std::string> &qmlImportPath,
                      const std::string &qmlUrl) = 0;
   };

} // namespace clap