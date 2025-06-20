#include <cstdint>

namespace jack {
    struct Tracer {
        uint8_t size;
        Tracer(uint8_t size) : size(size){

        };

        ~Tracer(){};
        Tracer(const Tracer& other) = delete;
        Tracer operator=(const Tracer& other) = delete;
        Tracer operator=(Tracer&& other) = delete;
    };
}
 