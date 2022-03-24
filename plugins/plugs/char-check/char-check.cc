#include <cstring>

#include "../../parameter-interpolator.hh"
#include "char-check.hh"

namespace clap {
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

   clap_process_status CharCheck::process(const clap_process *process) noexcept {
      return CLAP_PROCESS_SLEEP;
   }
} // namespace clap
