#include <memory>

#include "PanelVoice.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

namespace ospray {
namespace voice_plugin {

struct PluginVoice : public Plugin
{
  PluginVoice() : Plugin("Voice") {}

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.plugin_argc;
      const char **av = studioCommon.plugin_argv;

      std::string optPanelName = "Voice Panel";
      std::string configFilePath = "config/voice_settings.json";

      for (int i=0; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:voice:name") {
          optPanelName = av[i + 1];
          ++i;
        }
        else if (arg == "--plugin:voice:config") {
          configFilePath = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelVoice(ctx, optPanelName, configFilePath));
    }
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_voice()
{
  std::cout << "loaded plugin 'voice'!" << std::endl;
  return new PluginVoice();
}

} // namespace voice_plugin
} // namespace ospray
