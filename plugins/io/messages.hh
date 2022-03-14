#include <stdint.h>

#include <clap/clap.h>

namespace clap::messages {

   enum Type : uint32_t {
      // Client
      kCreateClientRequest,
      kCreateClientResponse,
      kDestroyClientRequest,
      kDestroyClientResponse,

      kAddImportPathRequest,
      kSetSkinRequest,

      // DSP->GUI
      kDefineParameterRequest,
      kParameterValueRequest,
      kUpdateTransportRequest,

      // GUI->DSP
      kParamBeginAdjustRequest,
      kParamAdjustRequest,
      kParamEndAdjustRequest,
      kSubscribeToTransportRequest,

      // Gui, Host->Plugin
      kSetScaleRequest,
      kSetScaleResponse,
      kCanResizeRequest,
      kCanResizeResponse,
      kGetSizeRequest,
      kGetSizeResponse,
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
      kOpenWindowRequest,
      kAttachWin32Request,
      kAttachCocoaRequest,
      kAttachX11Request,
      kAttachResponse,
      kSetTransientWin32Request,
      kSetTransientCocoaRequest,
      kSetTransientX11Request,
      kSetTransientResponse,

      // Gui, Plugin->Host
      kResizeRequest,
      kResizeResponse,
      kWindowClosedNotification,
   };

   struct CreateClientRequest final {
      static const constexpr Type type = clap::messages::kCreateClientRequest;
   };

   struct CreateClientResponse final {
      static const constexpr Type type = clap::messages::kCreateClientResponse;

      uint32_t clientId;
   };

   struct AddImportPathRequest final {
      static const constexpr Type type = clap::messages::kAddImportPathRequest;
      char importPath[4096];
   };

   struct SetSkinRequest final {
      static const constexpr Type type = clap::messages::kSetSkinRequest;
      char skinUrl[4096];
   };

   struct DestroyClientRequest final {
      static const constexpr Type type = clap::messages::kDestroyClientRequest;
   };

   struct DestroyClientResponse final {
      static const constexpr Type type = clap::messages::kCreateClientResponse;
   };

   struct ParamAdjustRequest final {
      static const constexpr Type type = clap::messages::kParamAdjustRequest;

      clap_id paramId;
      double value;
   };

   struct ParamBeginAdjustRequest final {
      static const constexpr Type type = clap::messages::kParamBeginAdjustRequest;

      clap_id paramId;
   };

   struct ParamEndAdjustRequest final {
      static const constexpr Type type = clap::messages::kParamEndAdjustRequest;

      clap_id paramId;
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

   struct CanResizeRequest final {
      static const constexpr Type type = kCanResizeRequest;
   };

   struct CanResizeResponse final {
      static const constexpr Type type = kCanResizeResponse;
      bool succeed;
   };

   struct GetSizeRequest final {
      static const constexpr Type type = kGetSizeRequest;
   };

   struct GetSizeResponse final {
      static const constexpr Type type = kGetSizeResponse;
      uint32_t width;
      uint32_t height;
      bool succeed;
   };

   struct SetSizeRequest final {
      static const constexpr Type type = kSetSizeRequest;
      uint32_t width;
      uint32_t height;
   };

   struct SetSizeResponse final {
      static const constexpr Type type = kSetSizeResponse;
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
      bool succeed;
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

   struct OpenWindowRequest final {
      static const constexpr Type type = kOpenWindowRequest;
   };

   struct AttachWin32Request final {
      static const constexpr Type type = kAttachWin32Request;
      clap_hwnd hwnd;
   };

   struct AttachX11Request final {
      static const constexpr Type type = kAttachX11Request;
      clap_xwnd window;
   };

   struct AttachCocoaRequest final {
      static const constexpr Type type = kAttachCocoaRequest;
      clap_nsview nsView;
   };

   struct AttachResponse final {
      static const constexpr Type type = kAttachResponse;
      bool succeed;
   };

   struct SetTransientWin32Request final {
      static const constexpr Type type = kSetTransientWin32Request;
      clap_hwnd hwnd;
   };

   struct SetTransientX11Request final {
      static const constexpr Type type = kSetTransientX11Request;
      clap_xwnd window;
   };

   struct SetTransientCocoaRequest final {
      static const constexpr Type type = kSetTransientCocoaRequest;
      clap_nsview nsView;
   };

   struct SetTransientResponse final {
      static const constexpr Type type = kSetTransientResponse;
      bool succeed;
   };


   struct WindowClosedNotification final {
      static const constexpr Type type = kWindowClosedNotification;
      bool wasDestroyed;
   };
} // namespace clap::messages