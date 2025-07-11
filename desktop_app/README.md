# Teleop LED Benchmarks
The goal of this project is to benchmark latency when teleoping an led across different network protocols.

# Notes on setting up dev environment

Install vcpkg. Set vcpkg root as an environment variable (with  cmake user presets).
To do this i can add a backup cmake user preset that is 
git ignored and instruct people to copy it.

On linux you need install glfw dependencies
https://www.glfw.org/docs/latest/compile_guide.html

# Shell command notes
Debug preset
```
cmake -DCMAKE_BUILD_TYPE=Debug --preset=vcpkg
```

Build
```
cmake --build build
```

Run app
```
build/MyApp
```

Run tests
```
ctest --test-dir build
```

Run filtered tests
```
./build/MyTest --gtest_filter=AppTest.*
```