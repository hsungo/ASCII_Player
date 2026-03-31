![C](https://img.shields.io/badge/Language-C-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

# ASCII Player

A terminal video player implemented in C that converts video frames into real-time ASCII art using FFmpeg.
> 10-level grayscale mapped to ASCII ( .:-=+*#%@)

## Getting Started
### Prerequisites
  * `gcc`、`make`
  * FFmpeg package : `sudo apt install -y libavcodec-dev libavformat-dev libswscale-dev libavutil-dev`
### Building
* #### Get `.exe` file
```bash
git clone https://github.com/hsungo/ASCII_Player.git
cd ASCII_Player
make build
```
* #### Player Usage
```bash
# run ascii player with picture
./ascii_player test.JPG

#run ascii player with video
./ascii_player test.mp4
```
