#pragma once

#include <memory>
#include <string>

namespace clap {
   class PathProvider {
   public:
      PathProvider(std::string pluginName);
      virtual ~PathProvider();

      static std::unique_ptr<PathProvider> create(const std::string &pluginPath,
                                                  const std::string &pluginName);

      std::string getQmlLibraryPath() const;
      std::string getQmlSkinPath() const;
      virtual std::string getGuiExecutable() const = 0;

      virtual bool isValid() const = 0;

   protected:
      virtual std::string getSkinDirectory() const = 0;
      virtual std::string getQmlLibDirectory() const = 0;

      const std::string _pluginName;
   };
} // namespace clap