#pragma once

// EXPERIMENTAL: floating plugin windows on Wayland.
//
// This is not part of CLAP: CLAP_WINDOW_API_WAYLAND exists, but clap_plugin_gui doesn't say
// what clap_window holds for it. This is a proposal, and it may change at any time.
//
// Model: the plugin opens its own connection to the compositor and creates its own
// xdg_toplevel. The host and the plugin don't share any Wayland object, only strings, so:
// - a bug on one side can't break the other side's connection
// - there are no rules about who reads or dispatches the connection
// - each side can use any Wayland implementation and any toolkit
// - the plugin's GUI can run in another process
//
// Window API: clap_plugin_gui.create() is called with CLAP_WINDOW_API_WAYLAND and
// is_floating = true.
//
// clap_plugin_gui.set_transient(): window->api is CLAP_WINDOW_API_WAYLAND and window->ptr is
// a `const char *` xdg-foreign handle, which is only valid during the call. The host gets it by
// exporting its toplevel with zxdg_exporter_v2.export_toplevel(), and must keep the
// zxdg_exported_v2 object alive while the plugin's window exists. The plugin imports it with
// zxdg_importer_v2.import_toplevel() and calls zxdg_imported_v2.set_parent_of() with its own
// toplevel, so the compositor keeps the plugin's window above the host's.
//
// clap_plugin_gui_wayland.set_activation_token(): a client can't focus or raise its own window
// on Wayland. Before calling clap_plugin_gui.show(), the host may create an xdg_activation_v1
// token from the user input which asked for the GUI (xdg_activation_token_v1.set_serial() and
// set_surface()), and pass it to the plugin. The plugin activates its window with it once.
//
// All the protocols are optional: without xdg-foreign the window has no parent, without
// xdg-activation the compositor decides about the focus. Without xdg-decoration (mutter), the
// plugin draws its own title bar.

#include <clap/clap.h>

#ifdef __cplusplus
extern "C" {
#endif

static const CLAP_CONSTEXPR char CLAP_EXT_GUI_WAYLAND[] = "clap.gui-wayland.experimental";

typedef struct clap_plugin_gui_wayland {
   // Gives an xdg_activation_v1 token for the next clap_plugin_gui.show().
   // The token is only valid during this call.
   // [main-thread & floating]
   void(CLAP_ABI *set_activation_token)(const clap_plugin_t *plugin, const char *token);
} clap_plugin_gui_wayland_t;

#ifdef __cplusplus
}
#endif
