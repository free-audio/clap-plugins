#pragma once

#include <memory>

#include <clap/helpers/plugin.hh>
#include <clap/helpers/param-queue.hh>
#include <clap/helpers/reducing-param-queue.hxx>

#include "parameters.hh"
#include "path-provider.hh"
#include "gui/abstract-gui-listener.h"

namespace clap {

   using PluginGlue = helpers::Plugin<helpers::MisbehaviourHandler::Terminate, helpers::CheckingLevel::Maximal>;
   extern template class helpers::Plugin<helpers::MisbehaviourHandler::Terminate, helpers::CheckingLevel::Maximal>;

   class AbstractGui;

   class CorePlugin : public PluginGlue, public AbstractGuiListener {
      using super = PluginGlue;

   public:
      CorePlugin(std::unique_ptr<PathProvider> &&pathProvider,
                 const clap_plugin_descriptor *desc,
                 const clap_host *host);
      ~CorePlugin() override;

      const PathProvider &pathProvider() const noexcept { return *_pathProvider; }

   protected:
      //-------------//
      // clap_plugin //
      //-------------//
      bool init() noexcept override;
      void initTrackInfo() noexcept;

      //------------------------//
      // clap_plugin_track_info //
      //------------------------//
      bool implementsTrackInfo() const noexcept override { return true; }
      void trackInfoChanged() noexcept override;

      //-------------------------//
      // clap_plugin_audio_ports //
      //-------------------------//
      bool implementsAudioPorts() const noexcept override;
      uint32_t audioPortsCount(bool is_input) const noexcept override;
      bool audioPortsInfo(uint32_t index,
                          bool is_input,
                          clap_audio_port_info *info) const noexcept override;
      uint32_t audioPortsConfigCount() const noexcept override;
      bool audioPortsGetConfig(uint32_t index,
                               clap_audio_ports_config *config) const noexcept override;
      bool audioPortsSetConfig(clap_id config_id) noexcept override;

      //--------------------//
      // clap_plugin_params //
      //--------------------//
      bool implementsParams() const noexcept override { return true; }

      uint32_t paramsCount() const noexcept override { return _parameters.count(); }

      bool paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept override {
         *info = _parameters.getByIndex(paramIndex)->info();
         return true;
      }

      virtual bool paramsValue(clap_id paramId, double *value) noexcept override {
         *value = _parameters.getById(paramId)->value();
         return true;
      }

      virtual bool paramsValueToText(clap_id paramId,
                                     double value,
                                     char *display,
                                     uint32_t size) noexcept override {
         // TODO
         return false;
      }

      virtual bool
      paramsTextToValue(clap_id param_id, const char *display, double *value) noexcept override {
         // TODO
         return false;
      }

      //-------------------//
      // clap_plugin_state //
      //-------------------//
      bool implementsState() const noexcept override { return true; }
      bool stateSave(clap_ostream *stream) noexcept override;
      bool stateLoad(clap_istream *stream) noexcept override;

      //-----------------//
      // clap_plugin_gui //
      //-----------------//
      bool implementsGui() const noexcept override { return true; }
      bool guiCreate() noexcept override;
      void guiDestroy() noexcept override;
      bool guiCanResize() const noexcept override { return false; }
      bool guiSize(uint32_t *width, uint32_t *height) noexcept override;
      void guiRoundSize(uint32_t *width, uint32_t *height) noexcept override {
         guiSize(width, height);
      }
      bool guiSetScale(double scale) noexcept override;
      void guiShow() noexcept override;
      void guiHide() noexcept override;
      void guiDefineParameters();

      //---------------------//
      // AbstractGuiListener //
      //---------------------//
      void onGuiPoll() override;
      void onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) override;
      void onGuiSetTransportIsSubscribed(bool isSubscribed) override;

      //---------------------//
      // clap_plugin_gui_x11 //
      //---------------------//
      bool implementsGuiX11() const noexcept override { return true; }
      bool guiX11Attach(const char *displayName, unsigned long window) noexcept override;

      //-----------------------//
      // clap_plugin_gui_win32 //
      //-----------------------//
      bool implementsGuiWin32() const noexcept override { return true; }
      bool guiWin32Attach(clap_hwnd window) noexcept override;

      //-----------------------//
      // clap_plugin_gui_cocoa //
      //-----------------------//
      bool implementsGuiCocoa() const noexcept override { return true; }
      bool guiCocoaAttach(void *nsView) noexcept override;

      //-------------------------------//
      // clap_plugin_gui_free_standing //
      //-------------------------------//
      bool implementsGuiFreeStanding() const noexcept override { return true; }
      bool guiFreeStandingOpen() noexcept override;

      //////////////////////
      // Cached Host Info //
      //////////////////////
      bool hasTrackInfo() const noexcept { return _hasTrackInfo; }
      const clap_track_info &trackInfo() const noexcept {
         assert(_hasTrackInfo);
         return _trackInfo;
      }
      uint32_t trackChannelCount() const noexcept {
         return _hasTrackInfo ? _trackInfo.channel_count : 2;
      }
      clap_chmap trackChannelMap() const noexcept {
         return _hasTrackInfo ? _trackInfo.channel_map : CLAP_CHMAP_STEREO;
      }

      //---------------------------//
      // clap_plugin_timer_support //
      //---------------------------//
      bool implementsTimerSupport() const noexcept override { return true; }
      void onTimer(clap_id timerId) noexcept override;

      //------------------------//
      // clap_plugin_fd_support //
      //------------------------//
      bool implementsPosixFdSupport() const noexcept override { return true; }
      void onPosixFd(int fd, int flags) noexcept override;

   protected:
      void guiAdjust(clap_id paramId, double value, uint32_t flags);
      void processGuiEvents(const clap_process *process);
      uint32_t
      processEvents(const clap_process *process, uint32_t &index, uint32_t count, uint32_t time);

      struct GuiToPluginValue {
         clap_id paramId;
         double value;
         uint32_t flags;
      };

      struct PluginToGuiValue {
         double value;
         double mod;
      };

      helpers::ParamQueue<GuiToPluginValue, 32> _guiToPluginQueue;
      helpers::ReducingParamQueue<clap_id, PluginToGuiValue> _pluginToGuiQueue;

      std::unique_ptr<PathProvider> _pathProvider;

      bool _hasTrackInfo = false;
      clap_track_info _trackInfo;

      std::vector<clap_audio_port_info> _audioInputs;
      std::vector<clap_audio_port_info> _audioOutputs;
      std::vector<clap_audio_ports_config> _audioConfigs;

      clap_id _guiTimerId = CLAP_INVALID_ID;
      std::unique_ptr<AbstractGui> _gui;

      Parameters _parameters;

      static const constexpr uint32_t _paramSmoothingDuration = 64;

      bool _isGuiTransportSubscribed = false;
      bool _hasTransportCopy = false;
      bool _hasTransport = false;
      clap_event_transport _transportCopy;
   };
} // namespace clap