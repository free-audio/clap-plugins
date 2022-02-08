#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <yas/binary_iarchive.hpp>
#include <yas/binary_oarchive.hpp>

#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hxx>

#include "core-plugin.hh"
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

   CorePlugin::~CorePlugin() {}

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

   bool CorePlugin::stateSave(clap_ostream *stream) noexcept {
      try {
         ClapOStream os(stream);
         yas::binary_oarchive<ClapOStream> ar(os);
         ar &_parameters;
      } catch (...) {
         return false;
      }
      return true;
   }

   bool CorePlugin::stateLoad(clap_istream *stream) noexcept {
      try {
         ClapIStream is(stream);
         yas::binary_iarchive<ClapIStream> ar(is);
         ar &_parameters;
      } catch (...) {
         return false;
      }
      return true;
   }

   bool CorePlugin::guiCreate() noexcept {
#ifdef CLAP_REMOTE_GUI
      auto guiPath = _pathProvider->getGuiExecutable();
      _guiFactory = RemoteGuiFactoryProxy::getInstance(guiPath);
#else
      _guiFactory = LocalGuiFactory::getInstance();
      if (!_guiFactory)
         _guiFactory = ThreadedGuiFactory::getInstance();
#endif

      std::vector<std::string> qmlPath;
      qmlPath.push_back(_pathProvider->getQmlLibraryPath());

      auto skinPath = _pathProvider->getQmlSkinPath();
      _gui = _guiFactory->createGuiClient(*this, qmlPath, skinPath);

      if (!_gui)
         return false;

      guiDefineParameters();
      return true;
   }

   void CorePlugin::guiDefineParameters() {
      for (int i = 0; i < paramsCount(); ++i) {
         clap_param_info info;
         paramsInfo(i, &info);
         _gui->defineParameter(info);
      }
   }

   void CorePlugin::guiDestroy() noexcept {
      if (_gui)
         _gui.reset();
   }

   bool CorePlugin::guiSize(uint32_t *width, uint32_t *height) noexcept {
      if (!_gui)
         return false;

      return _gui->size(width, height);
   }

   bool CorePlugin::guiSetScale(double scale) noexcept {
      if (_gui)
         return _gui->setScale(scale);
      return false;
   }

   void CorePlugin::guiShow() noexcept {
      if (_gui)
         _gui->show();
   }

   void CorePlugin::guiHide() noexcept {
      if (_gui)
         _gui->hide();
   }

   bool CorePlugin::guiX11Attach(const char *displayName, unsigned long window) noexcept {
      if (_gui)
         return _gui->attachX11(displayName, window);

      return false;
   }

   bool CorePlugin::guiWin32Attach(clap_hwnd window) noexcept {
      if (_gui)
         return _gui->attachWin32(window);

      return false;
   }

   bool CorePlugin::guiCocoaAttach(void *nsView) noexcept {
      if (_gui)
         return _gui->attachCocoa(nsView);

      return false;
   }

   bool CorePlugin::guiFreeStandingOpen() noexcept {
      // TODO
      return false;
   }

   //---------------------//
   // AbstractGuiListener //
   //---------------------//
   void CorePlugin::onGuiPoll() {
      _pluginToGuiQueue.consume([this](clap_id paramId, const CorePlugin::PluginToGuiValue &value) {
         _gui->updateParameter(paramId, value.value, value.mod);
      });

      if (_isGuiTransportSubscribed && _hasTransportCopy) {
         _gui->updateTransport(_transportCopy);
         _hasTransportCopy = false;
      }
   }

   void CorePlugin::onGuiParamAdjust(clap_id paramId, double value, uint32_t flags) {
      GuiToPluginValue item{paramId, value, flags};

      // very highly likely to succeed
      while (!_guiToPluginQueue.tryPush(item)) {
         if (_host.canUseParams())
            _host.paramsRequestFlush();

         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      if (!isProcessing() && _host.canUseParams())
         _host.paramsRequestFlush();
   }

   void CorePlugin::onGuiSetTransportIsSubscribed(bool isSubscribed) {}

   void CorePlugin::processInputParameterChange(const clap_event_header *hdr) {
      if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
         switch (hdr->type) {
         case CLAP_EVENT_PARAM_VALUE: {
            auto ev = reinterpret_cast<const clap_event_param_value *>(hdr);
            auto p = reinterpret_cast<Parameter *>(ev->cookie);
            if (p) {
               if (p->info().id != ev->param_id) {
                  std::ostringstream os;
                  os << "Host provided invalid cookie for param id: " << ev->param_id;
                  hostMisbehaving(os.str());
                  std::terminate();
               }

               if (isProcessing())
                  p->setValueSmoothed(ev->value, _paramSmoothingDuration);
               else
                  p->setValueImmediately(ev->value);
               _pluginToGuiQueue.set(p->info().id, {ev->value, p->modulation()});
            }
            break;
         }

         case CLAP_EVENT_PARAM_MOD: {
            auto ev = reinterpret_cast<const clap_event_param_mod *>(hdr);
            auto p = reinterpret_cast<Parameter *>(ev->cookie);
            if (p) {
               if (p->info().id != ev->param_id) {
                  std::ostringstream os;
                  os << "Host provided invalid cookie for param id: " << ev->param_id;
                  hostMisbehaving(os.str());
                  std::terminate();
               }

               p->setModulationSmoothed(ev->amount, _paramSmoothingDuration);
               _pluginToGuiQueue.set(p->info().id, {p->value(), ev->amount});
            }
            break;
         }
         }
      }
   }

   void CorePlugin::processGuiParameterChange(const clap_output_events *out) {
      GuiToPluginValue value;
      while (_guiToPluginQueue.tryPop(value)) {
         auto p = _parameters.getById(value.paramId);
         if (!p)
            return;

         if (isProcessing())
            p->setValueSmoothed(value.value, _paramSmoothingDuration);
         else
            p->setValueImmediately(value.value);

         clap_event_param_value ev;
         ev.header.time = 0;
         ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         ev.header.type = CLAP_EVENT_PARAM_VALUE;
         ev.header.size = sizeof(ev);
         ev.header.flags = value.flags;
         ev.param_id = value.paramId;
         ev.value = value.value;
         ev.channel = -1;
         ev.key = -1;
         ev.cookie = p;

         out->push_back(out, &ev.header);
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
            }
         }
      }

      return process->frames_count;
   }
} // namespace clap