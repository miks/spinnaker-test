#ifndef SRC_CAMERA_H_
#define SRC_CAMERA_H_

#include <string>
#include <iostream>
#include <unistd.h>
#include <queue>
#include <signal.h>
#include <thread>
#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

class Camera {
  public:
    Camera(bool& run);
    bool IsConnected();
    void Warmup();

    void Connect();
    bool ConnectDevice();
    void MaintainCaptureState();
    bool CloseCamera();
    bool OpenCamera();
    void IdleSleep();
    void Configure();
    void PrintDeviceInformation();
    void PrintDeviceConfiguration();
    void Capture(std::queue<Spinnaker::ImagePtr>& capture_queue);
    int FPS();
    void RegisterCaptureStart();
    void RegisterFrameCapture();
    std::string ConfigurationLabel(std::string str, const size_t num = 23, const char padding_char = ' ');

    double GetFloatProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map = NULL);
    int GetIntProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map = NULL);
    std::string GetStringProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map = NULL);
    bool GetBoolProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map = NULL);
    bool UpdateProperty(std::string config_name, int config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map = NULL);
    bool UpdateProperty(std::string config_name, double config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map = NULL);
    bool UpdateProperty(std::string config_name, std::string config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map = NULL);
    bool UpdateBoolProperty(std::string config_name, bool config_value, bool exclusive_write = false, Spinnaker::GenApi::INodeMap* map = NULL);
    bool IsWritable();
    bool EnsureWritability();
    void EnsureExclusiveWrite();
    void EndExclusiveWrite();

  private:
    // see https://www.ptgrey.com/tan/11174
    int SPINNAKER_BUFFER_SIZE = 10;

    double EXPOSURE_TIME = 1000;
    int IDLE_SLEEP = 25 * 1000; // 25ms

    Spinnaker::CameraPtr cam = 0;
    Spinnaker::SystemPtr system = 0;
    Spinnaker::CameraList cam_list;
    Spinnaker::GenApi::INodeMap* node_map_tl_device;
    Spinnaker::GenApi::INodeMap* node_map;
    Spinnaker::GenApi::INodeMap* stream_node_map;

    void Pause();
    void Resume();

    int fps = 0;

    double exposure_time;

    bool& run;
    bool camera_connected = false;
    bool capture = true;
    bool camera_open = false;
    long frame_counter = 0;
    std::time_t time_begin;

    const int CAMERA_RECONNECT_TIMEOUT = 5; // seconds
};

#endif  // SRC_CAMERA_H_
