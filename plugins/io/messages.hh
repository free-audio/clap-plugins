#include <stdint.h>

#include <clap/clap.h>

namespace clap::messages {

   enum Type : uint32_t {
      // DSP->GUI
      kDefineParameterRequest,
      kParameterValueRequest,
      kUpdateTransportRequest,

      // GUI->DSP
      kAdjustRequest,
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

   struct AdjustRequest final {
      static const constexpr Type type = clap::messages::kAdjustRequest;

      uint32_t clientId;
      clap_id paramId;
      double value;
      uint32_t flags;
   };

   struct DefineParameterRequest final {
      static const constexpr Type type = kDefineParameterRequest;
      uint32_t clientId;
      clap_param_info info;
   };

   struct UpdateTransportRequest final {
      static const constexpr Type type = kUpdateTransportRequest;
      uint32_t clientId;
      bool hasTransport;
      clap_event_transport transport;
   };

   struct SubscribeToTransportRequest final {
      static const constexpr Type type = kSubscribeToTransportRequest;
      uint32_t clientId;
      bool isSubscribed;
   };

   struct ParameterValueRequest final {
      static const constexpr Type type = kParameterValueRequest;
      uint32_t clientId;
      clap_id paramId;
      double value;
      double modulation;
   };

   struct SetScaleRequest final {
      static const constexpr Type type = kSetScaleRequest;
      uint32_t clientId;
      double scale;
   };

   struct SetScaleResponse final {
      static const constexpr Type type = kSetScaleResponse;
      uint32_t clientId;
      bool succeed;
   };

   struct SizeRequest final {
      static const constexpr Type type = kSizeRequest;
      uint32_t clientId;
   };

   struct SizeResponse final {
      static const constexpr Type type = kSizeResponse;
      uint32_t clientId;
      uint32_t width;
      uint32_t height;
      bool succeed;
   };

   struct RoundSizeRequest final {
      static const constexpr Type type = kRoundSizeRequest;
      uint32_t clientId;
      uint32_t width;
      uint32_t height;
   };

   struct RoundSizeResponse final {
      static const constexpr Type type = kRoundSizeResponse;
      uint32_t clientId;
      uint32_t width;
      uint32_t height;
   };

   struct SetSizeRequest final {
      static const constexpr Type type = kSetSizeRequest;
      uint32_t clientId;
      uint32_t width;
      uint32_t height;
   };

   struct SetSizeResponse final {
      static const constexpr Type type = kSetSizeResponse;
      uint32_t clientId;
   };

   struct ShowRequest final {
      static const constexpr Type type = kShowRequest;
      uint32_t clientId;
      bool succeed;
   };

   struct ShowResponse final {
      static const constexpr Type type = kShowResponse;
      uint32_t clientId;
   };

   struct HideRequest final {
      static const constexpr Type type = kHideRequest;
      uint32_t clientId;
      bool succeed;
   };

   struct HideResponse final {
      static const constexpr Type type = kHideResponse;
      uint32_t clientId;
   };

   struct DestroyRequest final {
      static const constexpr Type type = kDestroyRequest;
      uint32_t clientId;
   };

   struct DestroyResponse final {
      static const constexpr Type type = kDestroyResponse;
      uint32_t clientId;
   };

   struct ResizeRequest final {
      static const constexpr Type type = kResizeRequest;
      uint32_t clientId;
      uint32_t width;
      uint32_t height;
   };

   struct ResizeResponse final {
      static const constexpr Type type = kResizeResponse;
      uint32_t clientId;
   };

   struct AttachWin32Request final {
      static const constexpr Type type = kAttachWin32Request;
      uint32_t clientId;
      clap_hwnd hwnd;
   };

   struct AttachX11Request final {
      static const constexpr Type type = kAttachX11Request;
      uint32_t clientId;
      unsigned long window;
      char display[128];
   };

   struct AttachCocoaRequest final {
      static const constexpr Type type = kAttachCocoaRequest;
      uint32_t clientId;
      void *nsView;
   };

   struct AttachResponse final {
      static const constexpr Type type = kAttachResponse;
      uint32_t clientId;
      bool succeed;
   };
} // namespace clap::messages