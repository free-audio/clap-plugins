#include <stdint.h>

#include <clap/clap.h>

namespace clap::messages {

   enum Type : uint32_t {
      // DSP->GUI
      kDefineParameterRequest,
      kParameterValueRequest,
      kUpdateTransportRequest,

      // GUI->DSP
      kParamAdjustRequest,
      kSubscribeToTransportRequest,

      // Gui, Host->Plugin
      kSetScaleRequest,
      kSetScaleResponse,
      kSizeRequest,
      kSizeResponse,
      kRoundSizeRequest,
      kRoundSizeResponse,
      kSetSizeRequest,
      kSetSizeResponse,
      kShowRequest,
      kShowResponse,
      kHideRequest,
      kHideResponse,
      kDestroyRequest,
      kDestroyResponse,
      kAttachWin32Request,
      kAttachCocoaRequest,
      kAttachX11Request,
      kAttachResponse,

      // Gui, Plugin->Host
      kResizeRequest,
      kResizeResponse,
   };

   struct ParamAdjustRequest final {
      static const constexpr Type type = clap::messages::kParamAdjustRequest;

      clap_id paramId;
      double value;
      uint32_t flags;
   };

   struct DefineParameterRequest final {
      static const constexpr Type type = kDefineParameterRequest;
      clap_param_info info;
   };

   struct UpdateTransportRequest final {
      static const constexpr Type type = kUpdateTransportRequest;
      bool hasTransport;
      clap_event_transport transport;
   };

   struct SubscribeToTransportRequest final {
      static const constexpr Type type = kSubscribeToTransportRequest;
      bool isSubscribed;
   };

   struct ParameterValueRequest final {
      static const constexpr Type type = kParameterValueRequest;
      clap_id paramId;
      double value;
      double modulation;
   };

   struct SetScaleRequest final {
      static const constexpr Type type = kSetScaleRequest;
      double scale;
   };

   struct SetScaleResponse final {
      static const constexpr Type type = kSetScaleResponse;
      bool succeed;
   };

   struct SizeRequest final {
      static const constexpr Type type = kSizeRequest;
   };

   struct SizeResponse final {
      static const constexpr Type type = kSizeResponse;
      uint32_t width;
      uint32_t height;
      bool succeed;
   };

   struct RoundSizeRequest final {
      static const constexpr Type type = kRoundSizeRequest;
      uint32_t width;
      uint32_t height;
   };

   struct RoundSizeResponse final {
      static const constexpr Type type = kRoundSizeResponse;
      uint32_t width;
      uint32_t height;
   };

   struct SetSizeRequest final {
      static const constexpr Type type = kSetSizeRequest;
      uint32_t width;
      uint32_t height;
   };

   struct SetSizeResponse final {
      static const constexpr Type type = kSetSizeResponse;
   };

   struct ShowRequest final {
      static const constexpr Type type = kShowRequest;
   };

   struct ShowResponse final {
      static const constexpr Type type = kShowResponse;
      bool succeed;
   };

   struct HideRequest final {
      static const constexpr Type type = kHideRequest;
   };

   struct HideResponse final {
      static const constexpr Type type = kHideResponse;
      bool succeed;
   };

   struct DestroyRequest final {
      static const constexpr Type type = kDestroyRequest;
   };

   struct DestroyResponse final {
      static const constexpr Type type = kDestroyResponse;
   };

   struct ResizeRequest final {
      static const constexpr Type type = kResizeRequest;
      uint32_t width;
      uint32_t height;
   };

   struct ResizeResponse final {
      static const constexpr Type type = kResizeResponse;
   };

   struct AttachWin32Request final {
      static const constexpr Type type = kAttachWin32Request;
      clap_hwnd hwnd;
   };

   struct AttachX11Request final {
      static const constexpr Type type = kAttachX11Request;
      unsigned long window;
      char display[128];
   };

   struct AttachCocoaRequest final {
      static const constexpr Type type = kAttachCocoaRequest;
      void *nsView;
   };

   struct AttachResponse final {
      static const constexpr Type type = kAttachResponse;
      bool succeed;
   };
} // namespace clap::messages