#include "camera.hpp"

Camera::Camera(bool& run) : run( run ) {}

void Camera::Connect() {
  while(run && !camera_connected) {
    if (ConnectDevice()) {
      camera_connected = true;
      // Output device information
      PrintDeviceInformation();

      // Apply configuration
      Configure();

      // Review device configuration
      PrintDeviceConfiguration();
    }
    else {
      std::cout << "No camera detected" << std::endl;
      camera_connected = false;
    }

    if(!camera_connected) {
      std::cout << "reconnecting to camera in " << CAMERA_RECONNECT_TIMEOUT << " seconds" << std::endl;
      sleep(CAMERA_RECONNECT_TIMEOUT);
    }
  }
}

bool Camera::ConnectDevice() {
  // Retrieve singleton reference to system object
  system = Spinnaker::System::GetInstance();

  // Retrieve list of cameras from the system
  cam_list = system->GetCameras();

  // Finish if there are no cameras
  if (cam_list.GetSize() == 0) {

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    std::cout << "Camera is not connected" << std::endl;
    return false;
  }
  else {
    // Create shared pointer to camera
    cam = cam_list.GetByIndex(0);

    // Retrieve TL device node_map and print device information
    node_map_tl_device = &cam->GetTLDeviceNodeMap();

    // Initialize camera
    cam->Init();

    // Retrieve GenICam node_map
    node_map = &cam->GetNodeMap();

    // Retrieve GenICam stream node_map
    stream_node_map = &cam->GetTLStreamNodeMap();

    return true;
  }
}

void Camera::PrintDeviceInformation() {
  try {
    Spinnaker::GenApi::FeatureList_t features;
    Spinnaker::GenApi::CCategoryPtr category = node_map_tl_device->GetNode("DeviceInformation");

    if (Spinnaker::GenApi::IsAvailable(category) && Spinnaker::GenApi::IsReadable(category)) {
      std::cout << "Camera information" << std::endl;
      std::cout << "======================" << std::endl;
      category->GetFeatures(features);

      Spinnaker::GenApi::FeatureList_t::const_iterator it;
      for (it = features.begin(); it != features.end(); ++it) {
        Spinnaker::GenApi::CNodePtr feature_node = *it;
        Spinnaker::GenApi::CValuePtr value = (Spinnaker::GenApi::CValuePtr)feature_node;
        std::cout << ConfigurationLabel(feature_node->GetName().c_str()) << " " <<
            (Spinnaker::GenApi::IsReadable(value) ? value->ToString().c_str() : "Node not readable") << std::endl;
      }
    }
    else {
      std::cout << "Device control information not available" << std::endl;
    }
  }
  catch (Spinnaker::Exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
}

std::string Camera::ConfigurationLabel(std::string str, const size_t num, const char padding_char) {
  if(num > str.size())
    str.insert(str.end(), num - str.size(), padding_char);

  return str + ": ";
}

void Camera::PrintDeviceConfiguration() {
  try {
    std::cout << "Camera settings" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << ConfigurationLabel("Camera uptime") << " " << GetIntProperty("DeviceUptime") << std::endl;
    std::cout << ConfigurationLabel("Link uptime") << " " << GetIntProperty("LinkUptime") << std::endl;
    std::cout << ConfigurationLabel("Power supply voltage") << " " << GetFloatProperty("PowerSupplyVoltage") << std::endl;
    std::cout << ConfigurationLabel("Power supply current") << " " << GetFloatProperty("PowerSupplyCurrent") << std::endl;
    std::cout << ConfigurationLabel("ADC bit depth") << " " << GetStringProperty("AdcBitDepth") << std::endl;
    std::cout << ConfigurationLabel("Auto white balance") << " " << GetStringProperty("BalanceWhiteAuto") << std::endl;
    std::cout << ConfigurationLabel("Auto gain") << " " << GetStringProperty("GainAuto") << std::endl;
    std::cout << ConfigurationLabel("Acquisition mode") << " " << GetStringProperty("AcquisitionMode") << std::endl;
    std::cout << ConfigurationLabel("Pixel format") << " " << GetStringProperty("PixelFormat") << std::endl;
    std::cout << ConfigurationLabel("Reverse X") << " " << GetBoolProperty("ReverseX") << std::endl;
    std::cout << ConfigurationLabel("Reverse Y") << " " << GetBoolProperty("ReverseY") << std::endl;
    std::cout << ConfigurationLabel("Auto exposure") << " " << GetStringProperty("ExposureAuto") << std::endl;
    std::cout << ConfigurationLabel("Exposure time") << " " << GetFloatProperty("ExposureTime") << std::endl;
    std::cout << ConfigurationLabel("Frame rate") << " " << GetFloatProperty("AcquisitionFrameRate") << std::endl;
    std::cout << ConfigurationLabel("Gain") << " " << GetFloatProperty("Gain") << std::endl;
    std::cout << ConfigurationLabel("Width") << " " << GetIntProperty("Width") << std::endl;
    std::cout << ConfigurationLabel("Height") << " " << GetIntProperty("Height") << std::endl;
    std::cout << ConfigurationLabel("Offset X") << " " << GetIntProperty("OffsetX") << std::endl;
    std::cout << ConfigurationLabel("Offset Y") << " " << GetIntProperty("OffsetY") << std::endl;
    std::cout << ConfigurationLabel("Buffer handling mode") << " " << GetStringProperty("StreamBufferHandlingMode", stream_node_map) << std::endl;
    std::cout << ConfigurationLabel("Buffer count mode") << " " << GetStringProperty("StreamBufferCountMode", stream_node_map) << std::endl;
    std::cout << ConfigurationLabel("Buffer count") << " " << GetIntProperty("StreamBufferCountManual", stream_node_map) << std::endl;
    std::cout << "======================" << std::endl;
  }
  catch (Spinnaker::Exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
}

// check one of the settings whether it is writable
bool Camera::IsWritable() {
  Spinnaker::GenApi::CEnumerationPtr property = node_map->GetNode("PixelFormat");
  return Spinnaker::GenApi::IsAvailable(property) && Spinnaker::GenApi::IsWritable(property);
}

bool Camera::EnsureWritability() {
  // check write access or if camera is already opened
  // if camera is already opened than we can assume that write is possible
  if(IsWritable() || camera_open) {
    return true;
  }
  else {
    std::cout << "Camera write access has been locked" << std::endl;

    // simply starting and stopping acquisition somehow bring writability back
    // TODO: find correct way how to do this
    cam->BeginAcquisition();
    cam->EndAcquisition();

    if(IsWritable()) {
      return true;
    }
    else {
      std::cout << "Cannot get write access to camera" << std::endl;
      return false;
    }
  }
}

void Camera::Configure() {
  try {
    // force camera writability
    if(!EnsureWritability()) {
      return;
    }

    if(!UpdateProperty("AdcBitDepth", "Bit10", true)) {
      return;
    }

    if(!UpdateProperty("ExposureAuto", "Off", false)) {
      return;
    }

    if(!UpdateProperty("AcquisitionMode", "Continuous", true)) {
      return;
    }

    if(!UpdateProperty("StreamBufferHandlingMode", "OldestFirst", true, stream_node_map)) {
      return;
    }

    if(!UpdateProperty("StreamBufferCountMode", "Manual", true, stream_node_map)) {
      return;
    }

    if(!UpdateProperty("StreamBufferCountManual", SPINNAKER_BUFFER_SIZE, true, stream_node_map)) {
      return;
    }

    if(!UpdateProperty("ExposureTime", EXPOSURE_TIME, false)) {
      return;
    }

    // disable fixed frame rate to get correct max frame rate
    if(!UpdateBoolProperty("AcquisitionFrameRateEnable", false, false)) {
      return;
    }
  }
  catch (Spinnaker::Exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
}

double Camera::GetFloatProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  Spinnaker::GenApi::CFloatPtr property = map->GetNode(config_name.c_str());
  return property->GetValue();
}

int Camera::GetIntProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  Spinnaker::GenApi::CIntegerPtr property = map->GetNode(config_name.c_str());
  return property->GetValue();
}

std::string Camera::GetStringProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  Spinnaker::GenApi::CEnumerationPtr property = map->GetNode(config_name.c_str());
  return std::string(property->GetCurrentEntry()->GetSymbolic());
}

bool Camera::GetBoolProperty(std::string config_name, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  Spinnaker::GenApi::CBooleanPtr property = map->GetNode(config_name.c_str());
  return property->GetValue();
}

bool Camera::UpdateBoolProperty(std::string config_name, bool config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  // store update success
  bool result;

  Spinnaker::GenApi::CBooleanPtr property = map->GetNode(config_name.c_str());

  if(property->GetValue() == config_value) {
    result = true;
  }
  else {
    if(exclusive_write) {
      EnsureExclusiveWrite();
    }

    if (Spinnaker::GenApi::IsAvailable(property) && Spinnaker::GenApi::IsWritable(property)) {
      property->SetValue(config_value);
      result = true;
    }
    else {
      std::cout << "Cannot set: " << config_name << std::endl;
      result = false;
    }

    if(exclusive_write) {
      EndExclusiveWrite();
    }
  }

  return result;
}

bool Camera::UpdateProperty(std::string config_name, int config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  // store update success
  bool result;

  Spinnaker::GenApi::CIntegerPtr property = map->GetNode(config_name.c_str());

  if(property->GetValue() == config_value) {
    result = true;
  }
  else {
    if(exclusive_write) {
      EnsureExclusiveWrite();
    }

    if (Spinnaker::GenApi::IsAvailable(property) && Spinnaker::GenApi::IsWritable(property)) {
      property->SetValue(config_value);
      result = true;
    }
    else {
      std::cout << "Cannot set: " << config_name << std::endl;
      result = false;
    }

    if(exclusive_write) {
      EndExclusiveWrite();
    }
  }

  return result;
}

bool Camera::UpdateProperty(std::string config_name, double config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  // store update success
  bool result;

  Spinnaker::GenApi::CFloatPtr property = map->GetNode(config_name.c_str());

  if(property->GetValue() == config_value) {
    result = true;
  }
  else {
    if(exclusive_write) {
      EnsureExclusiveWrite();
    }

    if (Spinnaker::GenApi::IsAvailable(property) && Spinnaker::GenApi::IsWritable(property)) {
      property->SetValue(config_value);
      result = true;
    }
    else {
      std::cout << "Cannot set: " << config_name << std::endl;
      result = false;
    }

    if(exclusive_write) {
      EndExclusiveWrite();
    }
  }

  return result;
}

bool Camera::UpdateProperty(std::string config_name, std::string config_value, bool exclusive_write, Spinnaker::GenApi::INodeMap* map) {
  if(map == NULL) {
    map = node_map;
  }

  // store update success
  bool result;

  // retrieve the enumeration node from the node_map
  Spinnaker::GenApi::CEnumerationPtr property = map->GetNode(config_name.c_str());

  // stop here as we already have same value
  if(std::string(property->GetCurrentEntry()->GetSymbolic()) == config_value) {
    result = true;
  }
  else {
    if(exclusive_write) {
      EnsureExclusiveWrite();
    }

    if (Spinnaker::GenApi::IsAvailable(property) && Spinnaker::GenApi::IsWritable(property)) {
      // Retrieve the desired entry node from the enumeration node
      Spinnaker::GenApi::CEnumEntryPtr entryNode = property->GetEntryByName(config_value.c_str());

      if (Spinnaker::GenApi::IsAvailable(entryNode) && Spinnaker::GenApi::IsReadable(entryNode)) {
        // Retrieve the integer value from the entry node
        int64_t propertyValue = entryNode->GetValue();

        // Set integer as new value for enumeration node
        property->SetIntValue(propertyValue);
        result = true;
      }
      else {
        std::cout << config_name << " " <<  config_value << " not available" << std::endl;
        result = false;
      }
    }
    else {
      std::cout << "Cannot set: " << config_name << std::endl;
      result = false;
    }

    if(exclusive_write) {
      EndExclusiveWrite();
    }
  }

  return result;
}

void Camera::EnsureExclusiveWrite() {
  Pause();
  while(camera_open) {
    IdleSleep();
  }
}

void Camera::EndExclusiveWrite() {
  Resume();
}

void Camera::Capture(std::queue<Spinnaker::ImagePtr>& capture_queue) {
  RegisterCaptureStart();

  try {
    while(run) {
      if(!camera_connected) {
        Connect();
      }

      if(camera_connected) {
        MaintainCaptureState();
      }

      if(camera_connected && capture) {
        Spinnaker::ImagePtr raw_frame = cam->GetNextImage();

        if (raw_frame->IsIncomplete()) {
          std::cout << "Image incomplete with image status " << raw_frame->GetImageStatus() << std::endl;
        }
        else {
          Spinnaker::ImagePtr raw_frame_copy = Spinnaker::Image::Create();
          raw_frame_copy->DeepCopy(raw_frame);
          capture_queue.push(raw_frame_copy);
        }

        raw_frame->Release();

        RegisterFrameCapture();
      }
      else {
        IdleSleep();
      }
    }
  }
  catch (Spinnaker::Exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
  }

  if(camera_open) {
    CloseCamera();
  }

  // Deinitialize camera
  if(cam != 0) {
    cam->DeInit();
  }

  // Release reference to the camera
  cam = 0;

  // Clear camera list before releasing system
  cam_list.Clear();

  // Release system
  if(system != 0) {
    system->ReleaseInstance();
  }
}

bool Camera::CloseCamera() {
  cam->EndAcquisition();
  return true;
}

bool Camera::OpenCamera() {
  cam->BeginAcquisition();
  return true;
}

int Camera::FPS() {
  return fps;
}

void Camera::MaintainCaptureState() {
  if(!capture && camera_open) {
    camera_open = !CloseCamera();
  }
  else if(capture && !camera_open) {
    camera_open = OpenCamera();
  }
}

void Camera::IdleSleep() {
  usleep(IDLE_SLEEP); // 100 miliseconds or 0.1 second
}

void Camera::RegisterCaptureStart() {
  time_begin = std::time(0);
}

void Camera::RegisterFrameCapture() {
  // 1 second passed
  if ((std::time(0) - time_begin) >= 1) {
    fps = frame_counter;
    frame_counter = 0;
    time_begin = std::time(0);
  }

  frame_counter++;
}

void Camera::Pause() {
  capture = false;
}

void Camera::Resume() {
  capture = true;
}
