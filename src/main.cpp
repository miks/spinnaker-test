#include "main.hpp"

bool run = true;
const int QUEUE_READ_INTERVAL = 25 * 1000; // microseconds (every 25ms);
bool convert = false;

// capture queue
std::queue<Spinnaker::ImagePtr> capture_queue;

void HandleSigInt(int sig) {
  std::cout << "Exiting" << std::endl;
  run = false;
}

void Convert(Spinnaker::ImagePtr &spinnaker_frame) {
  std::cout << "Converting" << std::endl;
  Spinnaker::ImagePtr converted_image = spinnaker_frame->Convert(Spinnaker::PixelFormat_BGR8, Spinnaker::HQ_LINEAR);
}

void ConsumeQueue() {
  Spinnaker::ImagePtr image = capture_queue.front();
  capture_queue.pop();

  if(convert) {
    convert = false;
    Convert(image);
  }
}

void ProcessQueue() {
  while(run) {
    while(!capture_queue.empty()) {
      ConsumeQueue();
    }

    // pause a little bit between queue processing
    usleep(QUEUE_READ_INTERVAL); // 100 miliseconds or 0.1 second
  }
}

int MemoryUsage() {
  struct rusage r_usage;
  getrusage(RUSAGE_SELF, &r_usage);
  int memory_usage = r_usage.ru_maxrss;

  return memory_usage;
}

void Stat(Camera* camera) {
  while(run) {
    std::cout << "memory usage: " << MemoryUsage() << ", fps: " << camera->FPS() << std::endl;
    sleep(1);
  }
}

int mygetch() {
  struct termios oldt,newt;
  int ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}

int main(int argc, char **argv) {
  // Register shutdown signal
  signal(SIGINT, HandleSigInt);

  // Initialize camera object
  Camera* camera = new Camera(std::ref(run));

  // threads
  std::vector<std::thread> threads;

  // start queue processing thread
  threads.push_back(std::thread(ProcessQueue));

  // start camera capture thread
  threads.push_back(std::thread(&Camera::Capture, camera, std::ref(capture_queue)));

  // start stats thread
  threads.push_back(std::thread(Stat, std::ref(camera)));

  while(run) {
    int keyboard_input = mygetch();

    // detect "c" key pressed
    if(keyboard_input == 99) {
      convert = true;
    }

    sleep(1);
  }

  // wait for all threads to be finished
  std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

  return 0;
}
