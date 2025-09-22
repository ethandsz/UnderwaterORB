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

## Build this repo
```bash
bash Build.sh
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
