#include <obs/obs-module.h>
#include <obs/obs-frontend-api.h>
#include "vtp-source.h"
#include "vtp-output.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-vtp", "en-US")

MODULE_EXPORT const char* obs_module_description(void) {
    return "Video Transmission Protocol (VTP) integration for OBS Studio";
}

static void toggle_vtp_output(void*) {
    if (vtp_output_global_active()) {
        vtp_output_stop_global();
    } else {
        vtp_output_start_global("OBS-Program-Output");
    }
}

bool obs_module_load(void) {
    obs_register_source(&vtp_source_info);
    obs_register_output(&vtp_output_info);

    // Register a toggle action under OBS Tools menu
    obs_frontend_add_tools_menu_item("VTP Output (Start/Stop)", toggle_vtp_output, nullptr);
    return true;
}

void obs_module_unload(void) {
    if (vtp_output_global_active()) {
        vtp_output_stop_global();
    }
}
