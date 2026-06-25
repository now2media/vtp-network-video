#include "vtp-output.h"
#include "vtp-source.h"
#include <obs/obs-frontend-api.h>
#include <obs/obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("vtp-network-video", "en-US")

MODULE_EXPORT const char *obs_module_description(void) {
  return "VTP for OBS Studio";
}

static void toggle_vtp_output(void *) {
  if (vtp_output_global_active()) {
    vtp_output_stop_global();
  } else {
    vtp_output_start_global("OBS-Output");
  }
}

bool obs_module_load(void) {
  obs_register_source(&vtp_source_info);
  obs_register_output(&vtp_output_info);

  obs_frontend_add_tools_menu_item("VTP Output Start/Stop", toggle_vtp_output,
                                   nullptr);
  return true;
}

void obs_module_unload(void) {
  if (vtp_output_global_active()) {
    vtp_output_stop_global();
  }
}
