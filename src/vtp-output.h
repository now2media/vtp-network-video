#pragma once

#include <obs/obs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct obs_output_info vtp_output_info;

// Starts/Stops the main VTP Output globally
void vtp_output_start_global(const char* name);
void vtp_output_stop_global();
bool vtp_output_global_active();

#ifdef __cplusplus
}
#endif
