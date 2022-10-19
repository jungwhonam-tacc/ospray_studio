#include <memory>

#include "PanelStreamer.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

namespace ospray {
namespace streamer_plugin {

struct PluginStreamer : public Plugin
{
  PluginStreamer() : Plugin("Streamer") {}

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.plugin_argc;
      const char **av = studioCommon.plugin_argv;

      std::string optPanelName = "Streamer Panel";

      for (int i=0; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:streamer:name") {
          optPanelName = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelStreamer(ctx, optPanelName));
    }
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_streamer()
{
  std::cout << "loaded plugin 'streamer'!" << std::endl;
  return new PluginStreamer();
}

} // namespace streamer_plugin
} // namespace ospray
