# Speech interaction plugin for OSPRay Studio 
> This project is part of a larger project called [Immersive OSPray Studio](https://github.com/jungwhonam/ImmersiveOSPRay).

## Overview
We created a plugin for [OSPRay Studio v1.0.0](https://github.com/RenderKit/ospray-studio/releases/tag/v1.0.0) that allows users to navigate a 3D virtual environment using voice commands. Users can specify objects to detect in the current view by saying a command like "detect spheres." Once objects are detected, users can indicate which one to focus on by saying a command like "focus first." This command moves the virtual camera to zoom in on the corresponding object.

The plugin handles requests from [the Python script](https://github.com/JungWhoNam/text_guided_navigation). For instance, it captures data from OSPRay Studio and saves it to files for the Python script to process. Additionally, the plugin moves the virtual camera based on requests from the Python script. These operations are implemented in [the file](https://github.com/JungWhoNam/ospray_studio/blob/jungwho.nam-feature-plugin-chatbot/plugins/voice_plugin/PanelVoice.cpp).

> [!NOTE] 
> The current version assumes that the hosts running [the Python script](https://github.com/JungWhoNam/text_guided_navigation) and OSPRay Studio share a file system. When the script requests a screen capture, OSPRay Studio saves a screenshot in a predefined location and sends the file path to the script.

## Prerequisites
Before running `ospStudio` with the plugin, you need to run [the Python script](https://github.com/JungWhoNam/text_guided_navigation).

## Setup
```shell
# clone this branch
git clone -b jungwho.nam-feature-plugin-chatbot https://github.com/JungWhoNam/ospray_studio.git
cd ospray_studio

mkdir build
cd build
mkdir release
``` 

## CMake configuration and build
OSPRay Studio needs to be built with `-DBUILD_PLUGINS=ON` and `-DBUILD_PLUGIN_VOICE=ON` in CMake.

```shell
cmake -S .. \
-B release \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_PLUGINS=ON \
-DBUILD_PLUGIN_VOICE=ON

cmake --build release

cmake --install release
```

## Run `ospStudio` with the voice plugin

1. First, start [the Python script](https://github.com/JungWhoNam/text_guided_navigation).
2. Start `ospStudio` with the plugin.
```shell
./release/ospStudio \
--plugin voice \
--plugin:voice:config config/voice_settings.json
```
3. Go to `Plugins` > `Voice Panel` in the menu.
4. Click "Connect" button.

If connected, you will see "Connected to the server successfully" displayed on the Status sub-panel.

> To run `ospStudio` in a lightweight mode for testing, please use the following additional options: `--accumLimit 5 --renderer scivis --resolution 512x384`.

## Plugin configuration JSON file
When running `ospStudio`, you must specify the location of this JSON file using `--plugin:voice:config` flag. This file contains information about the Python script.

```json
{
    "ipAddress": "127.0.0.1",
    "portNumber": 8888
}
```
* `ipAddress` and `portNumber` are used for connecting the Python script.
  
> Here is [an example JSON file](./voice_settings.json).