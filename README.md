# vtp-network-video (OBS Studio VTP Integration Plugin)

**vtp-network-video** is a high-performance, open-source plugin for OBS Studio that enables bidirectional real-time video and audio transmission over local networks using the **VTP (Video Transmission Protocol)**. 

With VTP's highly optimized SIMD compression and zero-overhead transport pipeline, you can stream media between multiple OBS instances and external broadcast equipment with **sub-10ms latency**.

---

## ⚡ Key Features

* **VTP Client Source (Input):** Add ultra-low latency streams directly into your OBS scenes. Features auto-discovery (via UDP Multicast) and manual IP/Port fallback.
* **VTP Main Output (Sender):** Broadcast OBS's live Program feed (video and stereo audio) onto the local network with a single click.
* **Microsecond Audio/Video Sync:** Video frames and raw 16-bit PCM audio packets are linked using precision hardware timestamps to guarantee zero drift.
* **High Efficiency:** Utilizes TurboJPEG SIMD acceleration to perform lightweight encoding and decoding with negligible CPU footprint.

---

## 🛠️ Installation & Building (Ubuntu/Linux)

### 1. Install System Dependencies
To build the plugin from source, you will need the OBS development packages:

```bash
sudo apt update
sudo apt install -y cmake build-essential pkg-config libobs-dev libturbojpeg-dev
```

*Note for End Users: If you are distributing the compiled binary (`vtp-network-video.so`), users only need `libturbojpeg0` installed:*
```bash
sudo apt install libturbojpeg0
```

### 2. Add the VTP Core SDK
Create the SDK structure and copy the VTP files (compiled from [LIBVTP](https://github.com/now2media/libvtp)):

```bash
mkdir -p sdk/include sdk/lib
cp /path/to/vtp/include/vtp.h sdk/include/
cp /path/to/vtp/build/libvtp.so sdk/lib/
```

### 3. Build & Install
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
make install
```
*This installs the plugin directly into your local OBS plugins directory (`~/.config/obs-studio/plugins/vtp-network-video/bin/64bit/`).*

---

## 🚀 How to Use

### A. Receiving Video/Audio (VTP Input)
1. In OBS, click **`+`** under the **Sources** dock.
2. Select **VTP Client Source**.
3. Toggle **Use Auto-Discovery** to automatically discover the source by name (e.g., `Studio-Cam-01`), or enter the static **Server IP** and **Port** manually.

### B. Streaming OBS Output (VTP Output)
1. Go to the top menu in OBS: **Tools** -> **VTP Output (Start/Stop)**.
2. Click to toggle the broadcast.
3. The stream will broadcast under the name `OBS-Program-Output`. Other clients can now connect to this stream in real-time.

---

## 📄 License
This project is open-source and licensed under the MIT License.
