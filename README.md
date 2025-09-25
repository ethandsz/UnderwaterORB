# ORB-SLAM2 Live over UDP (BlueOS)

## Prerequisites
- OpenCV needs to be built with GStreamer support.
  - Verify:
    ```bash
    python3 - <<'PY'
    import cv2
    print('GStreamer: YES' in cv2.getBuildInformation())
    PY
    ```
    Should output `True`. If `False`, rebuild OpenCV with `-D WITH_GSTREAMER=ON` and install GStreamer runtimes. I built OpenCV from source via their github repo:

### building opencv

```
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1>
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake .. -D WITH_GSTREAMER=ON -D PYTHON3_EXECUTABLE=/usr/bin/python3 -D BUILD_opencv_python3=ON
```

ensure in the cmake output you see that it is building python3 bindings and it is building g streamer
```
--   Video I/O:
--     FFMPEG:                      YES
--       avcodec:                   YES (58.134.100)
--       avformat:                  YES (58.76.100)
--       avutil:                    YES (56.70.100)
--       swscale:                   YES (5.9.100)
--     GStreamer:                   YES (1.20.3)
--     v4l/v4l2:                    YES (linux/videodev2.h)
-- 
--   Parallel framework:            pthreads
-- 
--   Trace:                         YES (with Intel ITT(3.25.4))
-- 
--   Other third-party libraries:
--     Intel IPP:                   2022.2.0 [2022.2.0]
--            at:                   /home/student/opencv/build/3rdparty/ippicv/ippicv_lnx/icv
--     Intel IPP IW:                sources (2022.2.0)
--               at:                /home/student/opencv/build/3rdparty/ippicv/ippicv_lnx/iw
--     VA:                          NO
--     Lapack:                      NO
--     Eigen:                       YES (ver 3.4.0)
--     Custom HAL:                  YES (ipp (ver 0.0.1))
--     Protobuf:                    build (3.19.1)
--     Flatbuffers:                 builtin/3rdparty (23.5.9)
-- 
--   OpenCL:                        YES (no extra features)
--     Include path:                /home/student/opencv/3rdparty/include/opencl/1.2
--     Link libraries:              Dynamic load
-- 
--   Python 3:
--     Interpreter:                 /usr/bin/python3.10 (ver 3.10.12)
--     Libraries:                   /usr/lib/x86_64-linux-gnu/libpython3.10.so (ver 3.10.12)
--     Limited API:                 NO
--     numpy:                       /home/student/.local/lib/python3.10/site-packages/numpy/core/include (ver 1.26.4)
--     install path:                lib/python3.10/dist-packages/cv2/python-3.10
```

Then build the repo and install it
```
make
sudo checkinstall # follow instructions, change name to something like opencv-python-gstreamer
```

To uninstall:
```
dpkg -r opencv-python-gstreamer # or whatever name you named it
```

## Build this repo
```bash
bash Build.sh
```

## setup and start the robot

See instructions here:
```
https://bluerobotics.com/learn/bluerov2-assembly/#preliminary-vacuum-test
https://bluerobotics.com/learn/bluerov2-software-setup/#installing-qgroundcontrol
https://docs.bluerobotics.com/ping-viewer/#linux
```

## Bring up BlueOS camera
1. Connect the battery to the BlueROV.
2. Wait about 1–2 minutes for the second beep of the startup sequence. This starts BlueOS on the Raspberry Pi.
3. BlueOS then starts streaming the onboard camera to **udp:5600**.
4. With the tether attached to the ground station, you can verify the camera feed and settings at `http://blueos.local/`.

## Run ORB‑SLAM2 live
From the repo root:
```bash
cd Install/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
./mono_live ../../Source/Vocabulary/ORBvoc.txt ../../Source/Examples/Monocular/BLUEROV.yaml results.txt --gst
```
- Pangolin opens and the SLAM system starts.

## Notes
- `BLUEROV.yaml` matches the camera intrinsics and image size, I didnt have time to calibrate it but those settings should work ok.
- If your BlueOS stream uses a different port than 5600 internally, the `--gst` build in this repo already binds to the pipeline provided by the binary. If you need to force a custom pipeline (for example strictly using port 5000), run with an explicit GStreamer pipeline as the final argument:
  ```bash
  ./mono_live ../../Source/Vocabulary/ORBvoc.txt ../../Source/Examples/Monocular/BLUEROV.yaml test.txt --gst \
  "udpsrc port=5000 ! application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000 ! \
   rtpjitterbuffer latency=100 ! rtph264depay ! h264parse ! avdec_h264 ! \
   videoconvert ! video/x-raw,format=BGR ! appsink drop=true max-buffers=1 sync=false"
  ```

## Troubleshooting
- If you see `FATAL: cannot open video source with GStreamer`, first confirm the RTP stream with GStreamer directly:
  ```bash
  gst-launch-1.0 -v udpsrc port=5000 ! application/x-rtp,media=video,encoding-name=H264,payload=96 \
    ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
  ```
- If the image appears grayscale, ensure the pipeline forces `format=BGR` before `appsink` as shown above.
