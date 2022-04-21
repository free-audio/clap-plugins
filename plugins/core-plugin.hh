#pragma once

#include <memory>

#include <clap/helpers/param-queue.hh>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/reducing-param-queue.hxx>

#include "context.hh"
#include "gui/abstract-gui-factory.hh"
#include "gui/abstract-gui-listener.hh"
#include "intrusive-list.hh"
#include "parameters.hh"
#include "path-provider.hh"
#include "tuning-provider.hh"

#include "modules/root-module.hh"

namespace clap {

   using PluginGlue =
      helpers::Plugin<helpers::MisbehaviourHandler::Terminate, helpers::CheckingLevel::Maximal>;
   extern template class helpers::Plugin<helpers::MisbehaviourHandler::Terminate,
                                         helpers::CheckingLevel::Maximal>;

   class AbstractGui;
   class AbstractGuiFactory;
   class Module;

   class CorePlugin : public PluginGlue, public AbstractGuiListener {
      using super = PluginGlue;
      friend class Module;

      static const constexpr uint32_t max_frames = 256;

   public:
      CorePlugin(std::unique_ptr<PathProvider> &&pathProvider,
                 const clap_plugin_descriptor *desc,
                 const clap_host *host);
      ~CorePlugin() override;

      const PathProvider &pathProvider() const noexcept { return *_pathProvider; }
      const TuningProvider &tuningProvider() const noexcept { return _tuningProvider; }

   protected:
      //-------------//
      // clap_plugin //
      //-------------//
      bool init() noexcept override;
      void initTrackInfo() noexcept;

      bool
      activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
      void deactivate() noexcept override;
      bool startProcessing() noexcept override;
      void stopProcessing() noexcept override;
      void reset() noexcept override;

      clap_process_status process(const clap_process *process) noexcept final;
      clap_process_status
      processRange(const clap_process *process, uint32_t frameOffset, uint32_t frameCount) noexcept;
      void renderParameters(uint32_t frameCount) noexcept;

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

      bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;

      virtual bool paramsValue(clap_id paramId, double *value) noexcept override;

      virtual bool paramsValueToText(clap_id paramId,
                                     double value,
                                     char *display,
                                     uint32_t size) noexcept override;

      virtual bool
      paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;

      virtual void paramsFlush(const clap_input_events *in,
                               const clap_output_events *out) noexcept override;

      //-------------------//
      // clap_plugin_state //
      //-------------------//
      bool implementsState() const noexcept override { return true; }
      bool stateSave(const clap_ostream *stream) noexcept override;
      bool stateLoad(const clap_istream *stream) noexcept override;

      //-----------------//
      // clap_plugin_gui //
      //-----------------//
      bool implementsGui() const noexcept override;
      bool guiIsApiSupported(const char *api, bool isFloating) noexcept override;
      bool guiCreate(const char *api, bool isFloating) noexcept override;
      bool guiSetParent(const clap_window *window) noexcept override;
      bool guiSetTransient(const clap_window *window) noexcept override;
      void guiDestroy() noexcept override;
      bool guiCanResize() const noexcept override { return true; }
      bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override;
      bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
      bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept override;
      bool guiSetScale(double scale) noexcept override;
      bool guiShow() noexcept override;
      bool guiHide() noexcept override;
      void guiDefineParameters();

      //---------------------//
      // AbstractGuiListener //
      //---------------------//
      void onGuiPoll() override;
      void onGuiParamBeginAdjust(clap_id paramId) override;
      void onGuiParamAdjust(clap_id paramId, double value) override;
      void onGuiParamEndAdjust(clap_id paramId) override;
      void onGuiSetTransportIsSubscribed(bool isSubscribed) override;
      void onGuiWindowClosed(bool wasDestroyed) override;

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

      const char *trackAudioPortType() const noexcept {
         return _hasTrackInfo ? _trackInfo.audio_port_type : CLAP_PORT_STEREO;
      }

   protected:
      void guiAdjust(clap_id paramId, double value, uint32_t flags);
      void processGuiEvents(const clap_process *process);
      void processInputParameterChange(const clap_event_header *hdr);
      void processGuiParameterChange(const clap_output_events *out);
      uint32_t
      processEvents(const clap_process *process, uint32_t &index, uint32_t count, uint32_t time);

      struct GuiToPluginEvent {
         enum Type : uint8_t { Begin, Value, End };

         clap_id paramId;
         Type type;
         double value;
      };

      struct PluginToGuiValue {
         double value;
         double mod;
      };

      void pushGuiToPluginEvent(const GuiToPluginEvent &event);

      Parameter *addParameter(const clap_param_info &info, std::unique_ptr<ValueType> valueType) {
         auto p = _parameters.addParameter(info, std::move(valueType));
         _parameterValueToProcess.pushBack(&p->_mainVoice._valueToProcessHook);
         _parameterModulationToProcess.pushBack(&p->_mainVoice._modulationToProcessHook);
         _parameterModulatedValueToProcess.pushBack(&p->_mainVoice._modulatedValueToProcessHook);
         return p;
      }

      helpers::ParamQueue<GuiToPluginEvent, 32> _guiToPluginQueue;
      helpers::ReducingParamQueue<clap_id, PluginToGuiValue> _pluginToGuiQueue;

      std::unique_ptr<PathProvider> _pathProvider;

      bool _hasTrackInfo = false;
      clap_track_info _trackInfo;

      std::vector<clap_audio_port_info> _audioInputs;
      std::vector<clap_audio_port_info> _audioOutputs;
      std::vector<clap_audio_ports_config> _audioConfigs;

      clap_id _guiTimerId = CLAP_INVALID_ID;
      std::shared_ptr<AbstractGuiFactory> _guiFactory;
      std::unique_ptr<GuiHandle> _guiHandle;

      Parameters _parameters;
      IntrusiveList _parameterValueToProcess;
      IntrusiveList _parameterModulationToProcess;
      IntrusiveList _parameterModulatedValueToProcess;

      static const constexpr uint32_t _paramSmoothingDuration = 64;

      bool _isGuiTransportSubscribed = false;
      bool _hasTransportCopy = false;
      bool _hasTransport = false;
      clap_event_transport _transportCopy;

      Context _context;
      std::unique_ptr<Module> _rootModule;

      TuningProvider _tuningProvider;
   };
} // namespace clap