
// mono_udp.cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <sysexits.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "System.h"
/**/
/*static std::string default_gst_pipeline(int port = 5600) {*/
/*  // Receives RTP/H264 sent by BlueOS to the topside on UDP:5600*/
/*  // Bind locally; do NOT set a remote host here.*/
/*  return "udpsrc address=0.0.0.0 port=" + std::to_string(port) +*/
/*         " caps=application/x-rtp,media=video,encoding-name=H264,payload=96 ! "*/
/*         "rtpjitterbuffer ! rtph264depay ! h264parse ! avdec_h264 ! "*/
/*         "videoconvert ! appsink";*/
/*}*/
/**/

static std::string default_gst_pipeline(int port = 5600) {
  return
    "udpsrc port=" + std::to_string(port) + " ! "
    "application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000 ! "
    "rtpjitterbuffer latency=100 ! rtph264depay ! h264parse ! avdec_h264 ! "
    "videoconvert ! video/x-raw,format=BGR ! "
    "appsink drop=true max-buffers=1 sync=false";
}

static std::string default_ffmpeg_uri(int port = 5600) {
  // Listen on all interfaces. Large fifo to avoid packet drops.
  return "udp://@:" + std::to_string(port) +
         "?fifo_size=1000000&overrun_nonfatal=1";
}

int main(int argc, char** argv) {
  // Usage: mono_udp <path_to_ORBvoc.txt> <path_to_settings.yaml> <out_trajectory.txt> [--gst|--ffmpeg] [uri_or_pipeline]
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " ORBvoc.txt settings.yaml trajectory.txt [--gst|--ffmpeg] [uri_or_pipeline]\n";
    return EX_USAGE;
  }

  const std::string voc = argv[1];
  const std::string settings = argv[2];
  const std::string traj_out = argv[3];

  // Backend selection
  enum class Backend { GST, FFMPEG };
  Backend backend = Backend::GST; // default
  std::string source;

  if (argc >= 5) {
    std::string b = argv[4];
    if (b == "--ffmpeg") backend = Backend::FFMPEG;
    else if (b == "--gst") backend = Backend::GST;
    else source = b; // treat as uri/pipeline if user skipped flag
  }

  if (source.empty()) {
    source = (backend == Backend::GST) ? default_gst_pipeline(5600)
                                       : default_ffmpeg_uri(5600);
  } else if (argc >= 6) {
    // If both flag and explicit source given
    source = argv[5];
  }

  // ORB-SLAM2 init
  ORB_SLAM2::System SLAM(voc, settings, ORB_SLAM2::System::MONOCULAR, true);


  std::cout << "Opening stream`" << "\n";

  // Open stream
  cv::VideoCapture cap;
  if (backend == Backend::GST) {
    cap.open(source, cv::CAP_GSTREAMER);
  } else {
    cap.open(source, cv::CAP_FFMPEG);
  }

  if (!cap.isOpened()) {
    std::cerr << "FATAL: cannot open video source with "
              << ((backend == Backend::GST) ? "GStreamer" : "FFmpeg")
              << ":\n" << source << "\n";
    return EX_NOINPUT;
  }

  std::cout << "Opened stream. Backend=" << ((backend == Backend::GST) ? "GStreamer" : "FFmpeg") << "\n";

  // Start viewer in SLAM (blocking inside StartViewer), run capture+track in a thread
  std::atomic<bool> run{true};
  std::thread t([&](){
    cv::Mat frame;
    auto t0 = std::chrono::steady_clock::now();

    while (run.load()) {
      if (!cap.read(frame) || frame.empty()) {
        // brief backoff on stall
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }

      // Timestamp in seconds since start

      auto now = std::chrono::steady_clock::now();
      double tsec = std::chrono::duration<double>(now - t0).count();

      SLAM.TrackMonocular(frame, tsec);

      if (SLAM.isFinished()) break;
    }
  });

  // Viewer blocks until SLAM finishes or window closed
  SLAM.StartViewer();
  run.store(false);
  t.join();

  SLAM.Shutdown();
  SLAM.SaveTrajectoryTUM(traj_out);
  return EX_OK;
}
