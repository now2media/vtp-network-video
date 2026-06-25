# vtp-network-video

OBS Studio plugin for VTP (Video Transmission Protocol) streaming.

## Requirements

- libobs-dev
- libturbojpeg-dev

```bash
sudo apt install libobs-dev libturbojpeg-dev
```

## How to Build

Put `libvtp.so` and `vtp.h` under `sdk/lib/` and `sdk/include/` folders:

```bash
mkdir -p sdk/include sdk/lib
cp /path/to/vtp.h sdk/include/
cp /path/to/libvtp.so sdk/lib/
```

Build commands:

```bash
mkdir build && cd build
cmake ..
make
make install
```

Installs directly to `~/.config/obs-studio/plugins/vtp-network-video/bin/64bit/`.

## Usage

- Add **VTP Client Source** in OBS to receive streams.
- Use **Tools -> VTP Output Start/Stop** menu to stream OBS output.
