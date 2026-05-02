//Mmap example with simulation data

#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#pragma pack(push, 1)
struct CircuitEvent {
    double timestamp;   
    uint32_t signalID;  
    uint8_t newValue;   
}; // 13 bytes
#pragma pack(pop)

class SimulationLogManager {
private:
    const size_t MAX_EVENTS_PER_FILE = 1000000; // ~12.4 MB per segment
    const size_t segment_size = MAX_EVENTS_PER_FILE * sizeof(CircuitEvent);

    int current_fd = -1;
    int file_count = 0;
    CircuitEvent* current_map = nullptr;
    size_t event_index = 0;

    void rotate_file() {
        // 1. Unmap current file to free address space
        if (current_map) {
            munmap(current_map, segment_size);
            close(current_fd);
        }

        // 2. Create a new segment file
        std::string filename = "sim_part_" + std::to_string(file_count++) + ".bin";
        current_fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
        ftruncate(current_fd, segment_size);

        // 3. Map the new file
        void* ptr = mmap(NULL, segment_size, PROT_READ | PROT_WRITE, MAP_SHARED, current_fd, 0);
        if (ptr == MAP_FAILED) { throw std::runtime_error("mmap failed"); }
        
        current_map = static_cast<CircuitEvent*>(ptr);
        event_index = 0;
    }

public:
    SimulationLogManager() { rotate_file(); }

    void log_event(double ts, uint32_t id, uint8_t val) {
        if (event_index >= MAX_EVENTS_PER_FILE) {
            rotate_file();
        }

        current_map[event_index++] = {ts, id, val};
    }

    ~SimulationLogManager() {
        if (current_map) {
            munmap(current_map, segment_size);
            close(current_fd);
        }
    }
};

