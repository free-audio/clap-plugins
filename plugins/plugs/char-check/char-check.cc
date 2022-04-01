#include <cstring>

#include "char-check.hh"

namespace clap {
   class CharCheckModule final : public Module {
   public:
      CharCheckModule(CharCheck &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *CharCheck::descriptor() {
      static const char *features[] = {"validation", nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.char-check",
         "Character Check",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Plugin to check how well various languages are displayed",
         features};
      return &desc;
   }

   CharCheck::CharCheck(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "char-check"), descriptor(), host) {
      _rootModule = std::make_unique<CharCheckModule>(*this);

      addDumbParam("Hello");
      addDumbParam("こんにちは");
      addDumbParam("テレビ");
      addDumbParam("富士山");
      addDumbParam("שלום");
      addDumbParam("नमस्ते");
      addDumbParam("Привет");
      addDumbParam("سلام");
      addDumbParam("ਸਤ ਸ੍ਰੀ ਅਕਾਲ");
      addDumbParam("أهلا");
      addDumbParam("你好");
      addDumbParam("안녕하세요");
      addDumbParam("Pẹlẹ o");
      addDumbParam("Χαίρετε");
   }

   void CharCheck::addDumbParam(const char *name) {
      auto info = clap_param_info{
         _nextParamId++,
         CLAP_PARAM_IS_AUTOMATABLE,
         nullptr,
         "",
         "/",
         0,
         1,
         0,
      };
      snprintf(info.name, sizeof(info.name), "%s", name);
      _parameters.addParameter(info);
   }
} // namespace clap
