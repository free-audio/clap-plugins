#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <yas/binary_iarchive.hpp>
#include <yas/binary_oarchive.hpp>

#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hxx>

#include "container-of.hh"
#include "core-plugin.hh"
#include "modules/module.hh"
#include "stream-helper.hh"

#include "gui/abstract-gui.hh"

#ifdef CLAP_REMOTE_GUI
#   include "gui/remote-gui-factory-proxy.hh"
#else
#   include "gui/local-gui-factory.hh"
#   include "gui/threaded-gui-factory.hh"
#endif

namespace clap {

   template class helpers::Plugin<helpers::MisbehaviourHandler::Terminate,
                                  helpers::CheckingLevel::Maximal>;
   template class helpers::HostProxy<helpers::MisbehaviourHandler::Terminate,
                                     helpers::CheckingLevel::Maximal>;

   CorePlugin::CorePlugin(std::unique_ptr<PathProvider> &&pathProvider,
                          const clap_plugin_descriptor *desc,
                          const clap_host *host)
      : Plugin(desc, host), _pathProvider(std::move(pathProvider)) {
      assert(_pathProvider);
   }

   CorePlugin::~CorePlugin() = default;

   bool CorePlugin::init() noexcept {
      initTrackInfo();
      return true;
   }

   void CorePlugin::initTrackInfo() noexcept {
      checkMainThread();

      assert(!_hasTrackInfo);
      if (!_host.canUseTrackInfo())
         return;

      _hasTrackInfo = _host.trackInfoGet(&_trackInfo);
   }

   void CorePlugin::trackInfoChanged() noexcept {
      if (!_host.trackInfoGet(&_trackInfo)) {
         _hasTrackInfo = false;
         hostMisbehaving(
            "clap_host_track_info.get() failed after calling clap_plugin_track_info.changed()");
         return;
      }

      _hasTrackInfo = true;
   }

   bool CorePlugin::implementsAudioPorts() const noexcept { return true; }

   uint32_t CorePlugin::audioPortsCount(bool is_input) const noexcept {
      return is_input ? _audioInputs.size() : _audioOutputs.size();
   }

   bool CorePlugin::audioPortsInfo(uint32_t index,
                                   bool is_input,
                                   clap_audio_port_info *info) const noexcept {
      *info = is_input ? _audioInputs[index] : _audioOutputs[index];
      return true;
   }

   uint32_t CorePlugin::audioPortsConfigCount() const noexcept { return _audioConfigs.size(); }

   bool CorePlugin::audioPortsGetConfig(uint32_t index,
                                        clap_audio_ports_config *config) const noexcept {
      *config = _audioConfigs[index];
      return true;
   }

   bool CorePlugin::audioPortsSetConfig(clap_id config_id) noexcept { return false; }

   bool CorePlugin::stateSave(const clap_ostream *stream) noexcept {
      try {
         ClapOStream os(stream);
         yas::binary_oarchive<ClapOStream> ar(os);
         ar &_parameters;
      } catch (...) {
         return false;
      }
      return true;
   }

   bool CorePlugin::stateLoad(const clap_istream *stream) noexcept {
      try {
         ClapIStream is(stream);
         yas::binary_iarchive<ClapIStream> ar(is);
         ar &_parameters;
      } catch (...) {
         return false;
      }
      return true;
   }

   bool CorePlugin::implementsGui() const noexcept {
#ifdef CLAP_PLUGINS_HEADLESS
      return false;
#else
      return true;
#endif
   }

   bool CorePlugin::guiIsApiSupported(const char *api, bool isFloating) noexcept {
#if defined(CLAP_PLUGINS_HEADLESS)
      return false;
#else
#   if defined(__APPLE__) && defined(CLAP_REMOTE_GUI)
      return isFloating;
#   endif
      return true;
#endif
   }

   bool CorePlugin::guiCreate(const char *api, bool isFloating) noexcept {
#if defined(CLAP_PLUGINS_HEADLESS)
      return false;
#else

#   ifdef CLAP_REMOTE_GUI
      auto guiPath = _pathProvider->getGuiExecutable();
      _guiFactory = RemoteGuiFactoryProxy::getInstance(guiPath);
#   else
      _guiFactory = LocalGuiFactory::getInstance();
      if (!_guiFactory)
         _guiFactory = ThreadedGuiFactory::getInstance();
#   endif

      _guiHandle = _guiFactory->createGui(*this);

      if (!_guiHandle)
         return false;

      guiDefineParameters();

      auto skinPath = _pathProvider->getQmlSkinPath();
      _guiHandle->gui().addImportPath(_pathProvider->getQmlLibraryPath());
      _guiHandle->gui().setSkin(skinPath);

      return true;
#endif
   }

   bool CorePlugin::guiSetParent(const clap_window *window) noexcept {
#if defined(CLAP_PLUGINS_HEADLESS)
      return false;
#else
      if (!strcmp(CLAP_WINDOW_API_COCOA, window->api))
         return _guiHandle->gui().attachCocoa(window->cocoa);

      if (!strcmp(CLAP_WINDOW_API_WIN32, window->api))
         return _guiHandle->gui().attachWin32(window->win32);

      if (!strcmp(CLAP_WINDOW_API_X11, window->api))
         return _guiHandle->gui().attachX11(window->x11);

      return false;
#endif
   }

   bool CorePlugin::guiSetTransient(const clap_window *window) noexcept {
#if defined(CLAP_PLUGINS_HEADLESS)
      return false;
#else
      if (!_guiHandle->gui().openWindow())
         return false;

      if (!window)
         return true;

      if (!strcmp(CLAP_WINDOW_API_COCOA, window->api))
         _guiHandle->gui().setTransientCocoa(window->cocoa);
      else if (!strcmp(CLAP_WINDOW_API_WIN32, window->api))
         _guiHandle->gui().setTransientWin32(window->win32);
      else if (!strcmp(CLAP_WINDOW_API_X11, window->api))
         _guiHandle->gui().setTransientX11(window->x11);

      return true;
#endif
   }

   void CorePlugin::guiDefineParameters() {
      for (int i = 0; i < paramsCount(); ++i) {
         auto p = _parameters.getByIndex(i);
         _guiHandle->gui().defineParameter(p->info());
         _guiHandle->gui().updateParameter(p->info().id, p->value(), p->modulation());
      }
   }

   void CorePlugin::guiDestroy() noexcept { _guiHandle.reset(); }

   bool CorePlugin::guiGetSize(uint32_t *width, uint32_t *height) noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().getSize(width, height);
   }

   bool CorePlugin::guiAdjustSize(uint32_t *width, uint32_t *height) noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().roundSize(width, height);
   }

   bool CorePlugin::guiSetSize(uint32_t width, uint32_t height) noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().setSize(width, height);
   }

   bool CorePlugin::guiSetScale(double scale) noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().setScale(scale);
   }

   bool CorePlugin::guiShow() noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().show();
   }

   bool CorePlugin::guiHide() noexcept {
      if (!_guiHandle)
         return false;
      return _guiHandle->gui().hide();
   }

   //---------------------//
   // AbstractGuiListener //
   //---------------------//
   void CorePlugin::onGuiPoll() {
      _pluginToGuiQueue.consume([this](clap_id paramId, const CorePlugin::PluginToGuiValue &value) {
         _guiHandle->gui().updateParameter(paramId, value.value, value.mod);
      });

      if (_isGuiTransportSubscribed && _hasTransportCopy) {
         _guiHandle->gui().updateTransport(_transportCopy);
         _hasTransportCopy = false;
      }
   }

   void CorePlugin::pushGuiToPluginEvent(const GuiToPluginEvent &event) {
      // very highly likely to succeed
      while (!_guiToPluginQueue.tryPush(event)) {
         if (_host.canUseParams())
            _host.paramsRequestFlush();

         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      if (!isProcessing() && _host.canUseParams())
         _host.paramsRequestFlush();
   }

   void CorePlugin::onGuiSetTransportIsSubscribed(bool isSubscribed) {
      _isGuiTransportSubscribed = isSubscribed;
   }

   void CorePlugin::onGuiParamBeginAdjust(clap_id paramId) {
      GuiToPluginEvent item{paramId, GuiToPluginEvent::Begin, 0};
      pushGuiToPluginEvent(item);
   }

   void CorePlugin::onGuiParamEndAdjust(clap_id paramId) {
      GuiToPluginEvent item{paramId, GuiToPluginEvent::End, 0};
      pushGuiToPluginEvent(item);
   }

   void CorePlugin::onGuiParamAdjust(clap_id paramId, double value) {
      GuiToPluginEvent item{paramId, GuiToPluginEvent::Value, value};
      pushGuiToPluginEvent(item);
   }

   void CorePlugin::onGuiWindowClosed(bool wasDestroyed) {
      runOnMainThread([this, wasDestroyed] { _host.guiClosed(wasDestroyed); });
   }

   //------------------//
   // Audio Processing //
   //------------------//
   bool CorePlugin::activate(double sampleRate,
                             uint32_t minFrameCount,
                             uint32_t maxFrameCount) noexcept {
      _context.setSampleRate(sampleRate);

      auto setupBuffers = [sampleRate](auto &declAudio, auto &ctxAudio) {
         ctxAudio.resize(declAudio.size());

         for (uint32_t i = 0; i < declAudio.size(); ++i) {
            auto &info = declAudio[i];
            auto &buffer = ctxAudio[i];

            buffer =
               std::make_unique<AudioBuffer<double>>(info.channel_count, BLOCK_SIZE, sampleRate);
         }
      };

      setupBuffers(_audioInputs, _context.audioInputs);
      setupBuffers(_audioOutputs, _context.audioOutputs);

      return _rootModule->activate(sampleRate, maxFrameCount);
   }

   void CorePlugin::deactivate() noexcept { _rootModule->deactivate(); }

   bool CorePlugin::startProcessing() noexcept { return _rootModule->startProcessing(); }

   void CorePlugin::stopProcessing() noexcept { _rootModule->stopProcessing(); }

   void CorePlugin::reset() noexcept { _rootModule->reset(); }

   // Design:
   // - divide the block into chunks of up to N=256 frames
   // - render global parameters to audio buffers
   // - run the voice management:
   //   - prepare voice event queues
   //   - run the voices and eventually in parallel
   clap_process_status CorePlugin::process(const clap_process *process) noexcept {
      // This will process all the parameters changes coming from the plugin GUI
      processGuiEvents(process);

      const uint32_t evCount = process->in_events->size(process->in_events);
      uint32_t nextEvIndex = 0;
      uint32_t N = process->frames_count;
      clap_process_status status = CLAP_PROCESS_SLEEP;

      for (uint32_t i = 0; i < process->frames_count;) {
         N = processEvents(process, nextEvIndex, evCount, i);

         uint32_t numFramesToProcess = std::min(N - i, BLOCK_SIZE);
         // Process a range of frames
         status = processRange(process, i, numFramesToProcess);
         i += numFramesToProcess;
      }

      // Try to pass the queue to the plugin GUI
      _pluginToGuiQueue.producerDone();

      return status;
   }

   clap_process_status CorePlugin::processRange(const clap_process *process,
                                                uint32_t frameOffset,
                                                uint32_t frameCount) noexcept {
      renderParameters(frameCount);

      for (uint32_t i = 0; i < process->audio_inputs_count; ++i) {
         auto &moduleIn = _context.audioInputs[i];
         auto clapIn = &process->audio_inputs[i];
         moduleIn->fromClap(clapIn, frameOffset, frameCount);
      }

      auto status = _rootModule->process(_context, frameCount);

      for (uint32_t i = 0; i < process->audio_outputs_count; ++i) {
         auto &moduleOut = _context.audioOutputs[i];
         auto *clapOut = &process->audio_outputs[i];
         moduleOut->toClap(clapOut, frameOffset, frameCount);
      }

      return status;
   }

   void CorePlugin::renderParameters(uint32_t frameCount) noexcept {
      for (auto it = _parameterValueToProcess.begin(); !it.end();) {
         Parameter::Voice *paramVoice =
            containerOf(it.item(), &Parameter::Voice::_valueToProcessHook);
         ++it; // We increment immediately because param->renderValue() may unlink the param
         paramVoice->renderValue(frameCount);
      }

      for (auto it = _parameterModulationToProcess.begin(); !it.end();) {
         Parameter::Voice *paramVoice =
            containerOf(it.item(), &Parameter::Voice::_modulationToProcessHook);
         ++it; // We increment immediately because param->renderModulation() may unlink the param
         paramVoice->renderModulation(frameCount);
      }

      for (auto it = _parameterModulatedValueToProcess.begin(); !it.end();) {
         Parameter::Voice *paramVoice =
            containerOf(it.item(), &Parameter::Voice::_modulatedValueToProcessHook);
         ++it; // We increment immediately because param->renderModulation() may unlink the param
         paramVoice->renderModulatedValue(frameCount);
      }
   }

   void CorePlugin::processInputParameterChange(const clap_event_header *hdr) {
      if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
         switch (hdr->type) {
         case CLAP_EVENT_PARAM_VALUE: {
            auto ev = reinterpret_cast<const clap_event_param_value *>(hdr);
            auto p = reinterpret_cast<Parameter *>(ev->cookie);
            if (!p) [[unlikely]] {
               p = _parameters.getById(ev->param_id);
               if (!p) [[unlikely]]
                  break;
            }

            if (p->info().id != ev->param_id) [[unlikely]] {
               std::ostringstream os;
               os << "Host provided invalid cookie for param id: " << ev->param_id;
               hostMisbehaving(os.str());
               std::terminate();
            }

            if (isProcessing()) [[likely]]
               p->setValueSmoothed(ev->value, _paramSmoothingDuration);
            else
               p->setValueImmediately(ev->value);

            if (!p->_mainVoice._valueToProcessHook.isHooked())
               _parameterValueToProcess.pushBack(&p->_mainVoice._valueToProcessHook);
            if (!p->_mainVoice._modulatedValueToProcessHook.isHooked())
               _parameterModulatedValueToProcess.pushBack(
                  &p->_mainVoice._modulatedValueToProcessHook);

            _pluginToGuiQueue.set(p->info().id, {p->value(), p->modulation()});
            break;
         }

         case CLAP_EVENT_PARAM_MOD: {
            auto ev = reinterpret_cast<const clap_event_param_mod *>(hdr);
            auto p = reinterpret_cast<Parameter *>(ev->cookie);
            if (!p) [[unlikely]] {

               p = _parameters.getById(ev->param_id);
               if (!p) [[unlikely]]
                  break;
            }

            if (p->info().id != ev->param_id) [[unlikely]] {
               std::ostringstream os;
               os << "Host provided invalid cookie for param id: " << ev->param_id;
               hostMisbehaving(os.str());
               std::terminate();
            }

            const bool isGlobal = (ev->channel == -1 && ev->key == -1 && ev->port_index == -1);
            if (isGlobal) [[likely]] {
               if (isProcessing()) [[likely]]
                  p->setModulationSmoothed(ev->amount, _paramSmoothingDuration);
               else
                  p->setModulationImmediately(ev->amount);

               if (!p->_mainVoice._modulationToProcessHook.isHooked())
                  _parameterModulationToProcess.pushBack(&p->_mainVoice._modulationToProcessHook);
               if (!p->_mainVoice._modulatedValueToProcessHook.isHooked())
                  _parameterModulatedValueToProcess.pushBack(
                     &p->_mainVoice._modulatedValueToProcessHook);

               _pluginToGuiQueue.set(p->info().id, {p->value(), p->modulation()});
            } else {
               // TODO: find voice
            }

            break;
         }
         }
      }
   }

   void CorePlugin::processGuiParameterChange(const clap_output_events *out) {
      GuiToPluginEvent value;
      while (_guiToPluginQueue.tryPeek(value)) {
         auto p = _parameters.getById(value.paramId);
         if (!p) [[unlikely]]
            return;

         switch (value.type) {
         case GuiToPluginEvent::Value:
            [[likely]] {

               if (isProcessing()) [[likely]]
                  p->setValueSmoothed(value.value, _paramSmoothingDuration);
               else
                  p->setValueImmediately(value.value);

               if (!p->_mainVoice._valueToProcessHook.isHooked())
                  _parameterValueToProcess.pushBack(&p->_mainVoice._valueToProcessHook);
               if (!p->_mainVoice._modulatedValueToProcessHook.isHooked())
                  _parameterModulatedValueToProcess.pushBack(
                     &p->_mainVoice._modulatedValueToProcessHook);

               clap_event_param_value ev;
               ev.header.time = 0;
               ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
               ev.header.type = CLAP_EVENT_PARAM_VALUE;
               ev.header.size = sizeof(ev);
               ev.header.flags = 0;
               ev.param_id = value.paramId;
               ev.value = value.value;
               ev.channel = -1;
               ev.key = -1;
               ev.cookie = p;

               if (!out->try_push(out, &ev.header)) [[unlikely]]
                  return;
               break;
            }

         case GuiToPluginEvent::Begin:
         case GuiToPluginEvent::End: {
            auto param = _parameters.getById(value.paramId);
            if (!param) [[unlikely]]
               return;

            param->setHasGuiOverride(value.type == GuiToPluginEvent::Begin);

            clap_event_param_gesture ev;
            ev.header.time = 0;
            ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            ev.header.type = value.type == GuiToPluginEvent::Begin ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                                                   : CLAP_EVENT_PARAM_GESTURE_END;
            ev.header.size = sizeof(ev);
            ev.header.flags = 0;
            ev.param_id = value.paramId;

            if (!out->try_push(out, &ev.header)) [[unlikely]]
               return;
            break;
         }
         }

         _guiToPluginQueue.consume();
      }
   }

   void CorePlugin::paramsFlush(const clap_input_events *in,
                                const clap_output_events *out) noexcept {
      if (in) {
         const uint32_t N = in->size(in);
         for (uint32_t i = 0; i < N; ++i) {
            auto hdr = in->get(in, i);
            processInputParameterChange(hdr);
         }
      }

      processGuiParameterChange(out);
   }

   void CorePlugin::processGuiEvents(const clap_process *process) {
      processGuiParameterChange(process->out_events);

      /* We keep a copy of the transport to be able to send it to the plugin GUI */
      if (!_hasTransportCopy) {
         if (process->transport) {
            _hasTransport = true;
            _transportCopy = *process->transport;
         } else
            _hasTransport = false;

         _hasTransportCopy = true;
      }
   }

   uint32_t CorePlugin::processEvents(const clap_process *process,
                                      uint32_t &index,
                                      uint32_t count,
                                      uint32_t time) {
      for (; index < count; ++index) {
         auto hdr = process->in_events->get(process->in_events, index);

         if (hdr->time < time) {
            hostMisbehaving("Events must be ordered by time");
            std::terminate();
         }

         if (hdr->time > time) {
            // This event is in the future
            return std::min(hdr->time, process->frames_count);
         }

         if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
            switch (hdr->type) {
            case CLAP_EVENT_PARAM_VALUE:
            case CLAP_EVENT_PARAM_MOD:
               processInputParameterChange(hdr);
               break;

            case CLAP_EVENT_NOTE_ON:
               if (_rootModule->wantsNoteEvents()) {
                  auto ev = reinterpret_cast<const clap_event_note *>(hdr);
                  _rootModule->onNoteOn(*ev);
               }
               break;

            case CLAP_EVENT_NOTE_OFF:
               if (_rootModule->wantsNoteEvents()) {
                  auto ev = reinterpret_cast<const clap_event_note *>(hdr);
                  _rootModule->onNoteOff(*ev);
               }
               break;

            case CLAP_EVENT_NOTE_CHOKE:
               if (_rootModule->wantsNoteEvents()) {
                  auto ev = reinterpret_cast<const clap_event_note *>(hdr);
                  _rootModule->onNoteChoke(*ev);
               }
               break;

            case CLAP_EVENT_NOTE_EXPRESSION:
               if (_rootModule->wantsNoteEvents()) {
                  auto ev = reinterpret_cast<const clap_event_note_expression *>(hdr);
                  _rootModule->onNoteExpression(*ev);
               }
               break;

            case CLAP_EVENT_MIDI:
               // TODO
               break;

            case CLAP_EVENT_MIDI_SYSEX:
               // TODO
               break;

            case CLAP_EVENT_MIDI2:
               // TODO
               break;

            case CLAP_EVENT_TRANSPORT:
               // TODO
               break;
            }
         }
      }

      return process->frames_count;
   }

   bool CorePlugin::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept {
      auto param = _parameters.getByIndex(paramIndex);
      if (!param)
         return false;

      *info = param->info();
      return true;
   }

   bool CorePlugin::paramsValue(clap_id paramId, double *value) noexcept {
      auto param = _parameters.getById(paramId);
      if (!param)
         return false;

      *value = param->value();
      return true;
   }

   bool CorePlugin::paramsValueToText(clap_id paramId,
                                      double value,
                                      char *display,
                                      uint32_t size) noexcept {
      auto param = _parameters.getById(paramId);
      if (!param)
         return false;

      std::string text = param->valueType()->toText(value);
      snprintf(display, size, "%s", text.c_str());
      return true;
   }

   bool
   CorePlugin::paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept {
      auto param = _parameters.getById(paramId);
      if (!param)
         return false;

      *value = param->valueType()->fromText(display);
      return true;
   }

   Parameter *CorePlugin::addParameter(const clap_param_info &info,
                                       std::unique_ptr<ValueType> valueType) {
      auto p = _parameters.addParameter(info, std::move(valueType));
      _parameterValueToProcess.pushBack(&p->_mainVoice._valueToProcessHook);
      _parameterModulationToProcess.pushBack(&p->_mainVoice._modulationToProcessHook);
      _parameterModulatedValueToProcess.pushBack(&p->_mainVoice._modulatedValueToProcessHook);
      return p;
   }
} // namespace clap