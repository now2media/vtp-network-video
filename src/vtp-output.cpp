#include "vtp-output.h"
#include <vtp.h>
#include <iostream>
#include <mutex>
#include <atomic>

struct vtp_output_data {
    obs_output_t* output = nullptr;
    vtp_sender_t* sender = nullptr;
    std::string stream_name = "OBS-VTP-Output";
    std::atomic<bool> active{false};
};

static const char* vtp_output_get_name(void*) {
    return "VTP Send Output";
}

static void* vtp_output_create(obs_data_t* settings, obs_output_t* output) {
    auto* data = new vtp_output_data();
    data->output = output;
    
    const char* name = obs_data_get_string(settings, "stream_name");
    if (name && *name) {
        data->stream_name = name;
    }

    return data;
}

static void vtp_output_destroy(void* priv_data) {
    auto* data = static_cast<vtp_output_data*>(priv_data);
    if (data) {
        if (data->active) {
            vtp_sender_stop(data->sender);
            vtp_destroy_sender(data->sender);
        }
        delete data;
    }
}

static bool vtp_output_start(void* priv_data) {
    auto* data = static_cast<vtp_output_data*>(priv_data);
    if (!data || data->active) return false;

    // Get current OBS video information
    video_t* video = obs_output_video(data->output);
    const struct video_output_info* info = video_output_get_info(video);
    
    int width = (int)obs_output_get_width(data->output);
    int height = (int)obs_output_get_height(data->output);
    int fps = 30;
    if (info && info->fps_den > 0) {
        fps = (int)(info->fps_num / info->fps_den);
    }

    // Set up automatic conversion to RGBA
    video_scale_info conversion = {};
    conversion.format = VIDEO_FORMAT_RGBA;
    conversion.width = width;
    conversion.height = height;
    obs_output_set_video_conversion(data->output, &conversion);

    // Set up automatic conversion to 16-bit 48kHz Stereo PCM
    audio_convert_info audio_conversion = {};
    audio_conversion.format = AUDIO_FORMAT_16BIT;
    audio_conversion.samples_per_sec = 48000;
    audio_conversion.speakers = SPEAKERS_STEREO;
    obs_output_set_audio_conversion(data->output, &audio_conversion);

    // Initialize VTP sender
    data->sender = vtp_create_sender(data->stream_name.c_str(), width, height, fps, false, true);
    if (!data->sender || !vtp_sender_start(data->sender)) {
        std::cerr << "[OBS VTP Output] Failed to start VTP Sender" << std::endl;
        if (data->sender) {
            vtp_destroy_sender(data->sender);
            data->sender = nullptr;
        }
        return false;
    }

    // Dynamic quality setup
    vtp_sender_set_scale_quality(data->sender, 90);

    data->active = true;
    
    // Tell OBS to begin outputting video and audio
    if (!obs_output_begin_data_capture(data->output, 0)) {
        vtp_sender_stop(data->sender);
        vtp_destroy_sender(data->sender);
        data->sender = nullptr;
        data->active = false;
        return false;
    }

    std::cout << "[OBS VTP Output] Streaming started on port: " << vtp_sender_get_port(data->sender) << std::endl;
    return true;
}

static void vtp_output_stop(void* priv_data, uint64_t) {
    auto* data = static_cast<vtp_output_data*>(priv_data);
    if (data && data->active) {
        obs_output_end_data_capture(data->output);
        vtp_sender_stop(data->sender);
        vtp_destroy_sender(data->sender);
        data->sender = nullptr;
        data->active = false;
        std::cout << "[OBS VTP Output] Streaming stopped" << std::endl;
    }
}

static void vtp_output_raw_video(void* priv_data, struct video_data* frame) {
    auto* data = static_cast<vtp_output_data*>(priv_data);
    if (!data || !data->active || !data->sender) return;

    // Send the RGBA frame (pitch is frame->linesize[0])
    vtp_sender_send_frame(data->sender, frame->data[0], VTP_FORMAT_RGBA, frame->timestamp);
}

static void vtp_output_raw_audio(void* priv_data, struct audio_data* audio) {
    auto* data = static_cast<vtp_output_data*>(priv_data);
    if (!data || !data->active || !data->sender) return;

    // Send the 16-bit stereo PCM audio (channels=2, sample_rate=48000)
    vtp_sender_send_audio(data->sender, audio->data[0], (int)audio->frames, 48000, 2, audio->timestamp);
}

static void vtp_output_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, "stream_name", "OBS-VTP-Output");
}

struct obs_output_info vtp_output_info = {
    .id = "vtp_output",
    .flags = OBS_OUTPUT_VIDEO | OBS_OUTPUT_AUDIO,
    .get_name = vtp_output_get_name,
    .create = vtp_output_create,
    .destroy = vtp_output_destroy,
    .start = vtp_output_start,
    .stop = vtp_output_stop,
    .raw_video = vtp_output_raw_video,
    .raw_audio = vtp_output_raw_audio,
    .encoded_packet = nullptr,
    .update = nullptr,
    .get_defaults = vtp_output_defaults,
    .get_properties = nullptr
};

// ============================================================================
// Global Control
// ============================================================================
static obs_output_t* global_vtp_output = nullptr;

void vtp_output_start_global(const char* name) {
    if (global_vtp_output) return;

    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "stream_name", name);

    global_vtp_output = obs_output_create("vtp_output", "VTP Main Output", settings, nullptr);
    obs_data_release(settings);

    if (global_vtp_output) {
        obs_output_start(global_vtp_output);
    }
}

void vtp_output_stop_global() {
    if (!global_vtp_output) return;

    obs_output_stop(global_vtp_output);
    obs_output_release(global_vtp_output);
    global_vtp_output = nullptr;
}

bool vtp_output_global_active() {
    return global_vtp_output != nullptr && obs_output_active(global_vtp_output);
}
