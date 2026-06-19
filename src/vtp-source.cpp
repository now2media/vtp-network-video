#include "vtp-source.h"
#include <vtp.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <iostream>

struct vtp_source_data {
    obs_source_t* source = nullptr;
    vtp_receiver_t* receiver = nullptr;

    std::string ip;
    uint16_t port = 40425;
    std::string stream_name;
    bool use_discovery = false;

    std::thread receive_thread;
    std::thread audio_thread;
    std::atomic<bool> running{false};

    // Staging buffers for format conversion
    std::vector<uint8_t> video_conversion_buffer;
};

// Convert VTP pixel formats to OBS video formats
static enum video_format convert_vtp_format_to_obs(vtp_pixel_format_t format, bool& needs_conversion) {
    needs_conversion = false;
    switch (format) {
        case VTP_FORMAT_RGBA: return VIDEO_FORMAT_RGBA;
        case VTP_FORMAT_BGRA: return VIDEO_FORMAT_BGRA;
        case VTP_FORMAT_RGB:
            needs_conversion = true;
            return VIDEO_FORMAT_RGBA; // We will expand RGB to RGBA
        case VTP_FORMAT_BGR:
            needs_conversion = true;
            return VIDEO_FORMAT_BGRA; // We will expand BGR to BGRA
        default:
            return VIDEO_FORMAT_RGBA;
    }
}

static void video_receive_loop(vtp_source_data* data) {
    vtp_frame_t frame;
    while (data->running) {
        if (!data->receiver || !vtp_receiver_is_connected(data->receiver)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (vtp_receiver_receive_frame(data->receiver, &frame)) {
            bool needs_conversion = false;
            enum video_format obs_fmt = convert_vtp_format_to_obs(frame.format, needs_conversion);

            obs_source_frame obs_frame = {};
            obs_frame.width = frame.width;
            obs_frame.height = frame.height;
            obs_frame.format = obs_fmt;
            obs_frame.timestamp = frame.timestamp_ns;

            if (needs_conversion) {
                // Convert 24-bit RGB/BGR to 32-bit RGBA/BGRA
                size_t num_pixels = frame.width * frame.height;
                data->video_conversion_buffer.resize(num_pixels * 4);

                const uint8_t* src = frame.data;
                uint8_t* dst = data->video_conversion_buffer.data();

                for (size_t i = 0; i < num_pixels; ++i) {
                    dst[i * 4 + 0] = src[i * 3 + 0]; // R or B
                    dst[i * 4 + 1] = src[i * 3 + 1]; // G
                    dst[i * 4 + 2] = src[i * 3 + 2]; // B or R
                    dst[i * 4 + 3] = 255;            // Alpha opaque
                }

                obs_frame.data[0] = data->video_conversion_buffer.data();
                obs_frame.linesize[0] = frame.width * 4;
            } else {
                obs_frame.data[0] = const_cast<uint8_t*>(frame.data);
                obs_frame.linesize[0] = frame.width * ((frame.format == VTP_FORMAT_RGBA || frame.format == VTP_FORMAT_BGRA) ? 4 : 3);
            }

            obs_source_output_video(data->source, &obs_frame);
        }
    }
}

static void audio_receive_loop(vtp_source_data* data) {
    vtp_audio_frame_t audio_frame;
    while (data->running) {
        if (!data->receiver || !vtp_receiver_is_connected(data->receiver)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (vtp_receiver_receive_audio(data->receiver, &audio_frame)) {
            obs_source_audio obs_audio = {};
            obs_audio.data[0] = const_cast<uint8_t*>(audio_frame.data);
            obs_audio.frames = audio_frame.sample_count;
            obs_audio.speakers = (audio_frame.channels == 2) ? SPEAKERS_STEREO : SPEAKERS_MONO;
            obs_audio.samples_per_sec = audio_frame.sample_rate;
            obs_audio.format = AUDIO_FORMAT_16BIT; // VTP uses 16-bit Signed PCM
            obs_audio.timestamp = audio_frame.timestamp_ns;

            obs_source_output_audio(data->source, &obs_audio);
        }
    }
}

static const char* vtp_get_name(void*) {
    return "VTP Client Source";
}

static void vtp_destroy(void* priv_data) {
    auto* data = static_cast<vtp_source_data*>(priv_data);
    if (data) {
        data->running = false;
        if (data->receiver) {
            vtp_receiver_disconnect(data->receiver);
        }

        if (data->receive_thread.joinable()) data->receive_thread.join();
        if (data->audio_thread.joinable()) data->audio_thread.join();

        if (data->receiver) {
            vtp_destroy_receiver(data->receiver);
        }
        delete data;
    }
}

static void* vtp_create(obs_data_t* settings, obs_source_t* source) {
    auto* data = new vtp_source_data();
    data->source = source;
    data->receiver = vtp_create_receiver();

    obs_source_update(source, settings);

    data->running = true;
    data->receive_thread = std::thread(video_receive_loop, data);
    data->audio_thread = std::thread(audio_receive_loop, data);

    return data;
}

static void vtp_update(void* priv_data, obs_data_t* settings) {
    auto* data = static_cast<vtp_source_data*>(priv_data);
    if (!data) return;

    // Disconnect old session
    if (vtp_receiver_is_connected(data->receiver)) {
        vtp_receiver_disconnect(data->receiver);
    }

    data->ip = obs_data_get_string(settings, "ip");
    data->port = (uint16_t)obs_data_get_int(settings, "port");
    data->stream_name = obs_data_get_string(settings, "stream_name");
    data->use_discovery = obs_data_get_bool(settings, "use_discovery");

    if (data->use_discovery && !data->stream_name.empty()) {
        // Connect via discovery
        vtp_listener_t* listener = vtp_create_listener();
        vtp_listener_start(listener);
        
        // Wait briefly for discovery advertisements
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        bool connected = vtp_receiver_connect_by_name(data->receiver, listener, data->stream_name.c_str());
        if (!connected) {
            std::cerr << "[OBS VTP] Discovery connection failed for stream: " << data->stream_name << std::endl;
        }

        vtp_listener_stop(listener);
        vtp_destroy_listener(listener);
    } else if (!data->ip.empty()) {
        // Connect directly via IP/Port
        vtp_receiver_connect(data->receiver, data->ip.c_str(), data->port);
    }
}

static obs_properties_t* vtp_properties(void*) {
    obs_properties_t* props = obs_properties_create();

    obs_properties_add_bool(props, "use_discovery", "Use Auto-Discovery");
    obs_properties_add_text(props, "stream_name", "Stream Name (for Discovery)", OBS_TEXT_DEFAULT);
    
    obs_properties_add_text(props, "ip", "Server IP (for Manual)", OBS_TEXT_DEFAULT);
    obs_properties_add_int(props, "port", "Server Port (for Manual)", 1024, 65535, 1);

    return props;
}

static void vtp_defaults(obs_data_t* settings) {
    obs_data_set_default_bool(settings, "use_discovery", true);
    obs_data_set_default_string(settings, "stream_name", "VTP-Camera-1");
    obs_data_set_default_string(settings, "ip", "127.0.0.1");
    obs_data_set_default_int(settings, "port", 40425);
}

struct obs_source_info vtp_source_info = {
    .id = "vtp_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO,
    .get_name = vtp_get_name,
    .create = vtp_create,
    .destroy = vtp_destroy,
    .get_width = nullptr,
    .get_height = nullptr,
    .get_defaults = vtp_defaults,
    .get_properties = vtp_properties,
    .update = vtp_update,
    .activate = nullptr,
    .deactivate = nullptr,
    .show = nullptr,
    .hide = nullptr,
    .video_tick = nullptr,
    .video_render = nullptr,
    .filter_video = nullptr,
    .filter_audio = nullptr,
    .enum_active_sources = nullptr,
    .save = nullptr,
    .load = nullptr,
    .mouse_click = nullptr,
    .mouse_move = nullptr,
    .mouse_wheel = nullptr,
    .focus = nullptr,
    .key_click = nullptr,
    .filter_remove = nullptr,
    .type_data = nullptr,
    .free_type_data = nullptr,
    .audio_render = nullptr,
    .enum_all_sources = nullptr,
    .transition_start = nullptr,
    .transition_stop = nullptr,
    .get_defaults2 = nullptr,
    .get_properties2 = nullptr,
    .audio_mix = nullptr,
    .icon_type = OBS_ICON_TYPE_CAMERA
};
