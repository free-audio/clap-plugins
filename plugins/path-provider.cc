#include <cassert>
#include <cstring>
#include <filesystem>
#include <regex>
#include <sstream>

#include "path-provider.hh"

namespace clap {

   std::string PathProvider::getQmlLibraryPath() const {
#ifdef CLAP_PLUGINS_EMBED_QML
      return "qrc:/qml";
#else
      return getQmlLibDirectory();
#endif
   }

   std::string PathProvider::getQmlSkinPath() const {
      std::ostringstream os;
#ifdef CLAP_PLUGINS_EMBED_QML
      os << "qrc:/qml/clap/skins/" << _pluginName;
#else
      os << getSkinDirectory();
#endif
      os << "/main.qml";
      return os.str();
   }

   class DummyPathProvider final : public PathProvider {
   public:
      DummyPathProvider(std::string pluginName) : PathProvider(std::move(pluginName)) {}

      virtual std::string getGuiExecutable() const { std::terminate(); }

      virtual bool isValid() const { return true; }

   protected:
      virtual std::string getSkinDirectory() const { std::terminate(); }
      virtual std::string getQmlLibDirectory() const { std::terminate(); }
   };

   class LinuxPathProvider final : public PathProvider {
   public:
      LinuxPathProvider(const std::string &pluginPath, const std::string &pluginName)
         : PathProvider(pluginName), prefix_(computePrefix(pluginPath)) {}

      static std::string computePrefix(const std::string &pluginPath) {
         static const std::regex r("(/.*)/lib/clap/.*$", std::regex::optimize);

         std::smatch m;
         if (!std::regex_match(pluginPath, m, r))
            return "/usr";
         return m[1];
      }

      std::string getGuiExecutable() const override {
         return (prefix_ / "bin/clap-gui").generic_string();
      }

      bool isValid() const noexcept override { return !prefix_.empty(); }

   protected:
      std::string getSkinDirectory() const override {
         return (prefix_ / "lib/clap/qml" / _pluginName).generic_string();
      }

      std::string getQmlLibDirectory() const override {
         return (prefix_ / "lib/clap/qml/lib").generic_string();
      }

   private:
      const std::filesystem::path prefix_;
   };

   class DevelopmentPathProvider final : public PathProvider {
   public:
      DevelopmentPathProvider(const std::string &pluginPath, const std::string &pluginName)
         : PathProvider(pluginName), _srcRoot(computeSrcRoot(pluginPath)),
           _buildRoot(computeBuildRoot(pluginPath)), _multiPrefix(computeMultiPrefix(pluginPath)) {}

      static std::string computeSrcRoot(const std::string &pluginPath) {
         static const std::regex r("^(.*)/builds/.*$", std::regex::optimize);

         std::smatch m;
         if (!std::regex_match(pluginPath, m, r))
            return "";
         return m[1];
      }

      static std::string computeBuildRoot(const std::string &pluginPath) {
         static const std::regex r("^((.*)/builds/.*)/plugins/.*\\.clap$", std::regex::optimize);

         std::smatch m;
         if (!std::regex_match(pluginPath, m, r))
            return "";
         return m[1];
      }

      static std::string computeMultiPrefix(const std::string &pluginPath) {
         static const std::regex r("^.*/builds/.*/plugins/(.*)/.*\\.clap$", std::regex::optimize);

         std::smatch m;
         if (!std::regex_match(pluginPath, m, r))
            return "";
         return m[1];
      }

      std::string getGuiExecutable() const override {
         return (_buildRoot / "plugins/gui" / _multiPrefix / "clap-gui").generic_string();
      }

      std::string getSkinDirectory() const override {
         return (_srcRoot / "plugins/gui/qml/clap/skins" / _pluginName).generic_string();
      }

      std::string getQmlLibDirectory() const override {
         return (_srcRoot / "plugins/gui/qml").generic_string();
      }

      bool isValid() const noexcept override { return !_srcRoot.empty() && !_buildRoot.empty(); }

   private:
      const std::filesystem::path _srcRoot;
      const std::filesystem::path _buildRoot;
      const std::string _multiPrefix;
   };

   PathProvider::PathProvider(std::string pluginName)
      : _pluginName(std::move(pluginName))
   {
   }

   PathProvider::~PathProvider() = default;

   std::unique_ptr<PathProvider> PathProvider::create(const std::string &_pluginPath,
                                                      const std::string &pluginName) {

      auto pluginPath = std::filesystem::absolute(_pluginPath).generic_string();

#ifdef CLAP_PLUGINS_EMBED_QML
      return std::make_unique<DummyPathProvider>(pluginName);
#else

      auto devPtr = std::make_unique<DevelopmentPathProvider>(pluginPath, pluginName);
      if (devPtr->isValid())
         return std::move(devPtr);

#ifdef __unix

      auto ptr = std::make_unique<LinuxPathProvider>(pluginPath, pluginName);
      if (ptr->isValid())
         return std::move(ptr);

#elif defined(_WIN32)

#endif
#endif
      // TODO
      return nullptr;
   }
} // namespace clap