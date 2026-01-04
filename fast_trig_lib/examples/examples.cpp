// examples.cpp - Example usage of the FastTrig library

#include "fast_trig.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace FastTrig;

// Example 1: Robot navigation
class RobotNavigator {
    using Trig = Trig128;
    
public:
    struct Position {
        int32_t x, y;  // millimeters
    };
    
    struct Target {
        Position pos;
        uint16_t heading;   // Direction to target
        int32_t distance;   // Distance to target
    };
    
    Target calculate_target(Position current, Position goal) {
        int32_t dx = goal.x - current.x;
        int32_t dy = goal.y - current.y;
        
        return {
            goal,
            Trig::atan2(dy, dx),
            Trig::magnitude(dx, dy)
        };
    }
    
    Position move(Position current, uint16_t heading, int32_t distance) {
        int16_t cos_h = Trig::cos(heading);
        int16_t sin_h = Trig::sin(heading);
        
        return {
            current.x + (distance * cos_h) / 16384,
            current.y + (distance * sin_h) / 16384
        };
    }
};

// Example 2: Servo control for robot arm
class ServoController {
    using Trig = Trig256;  // Higher precision for servo control
    
public:
    struct JointAngles {
        uint16_t shoulder;
        uint16_t elbow;
        uint16_t wrist;
    };
    
    struct EndEffector {
        int16_t x, y, z;  // Position in mm
    };
    
    // Forward kinematics
    EndEffector forward_kinematics(JointAngles angles, int16_t l1, int16_t l2) {
        int16_t cos_s = Trig::cos(angles.shoulder);
        int16_t sin_s = Trig::sin(angles.shoulder);
        int16_t cos_e = Trig::cos(angles.elbow);
        int16_t sin_e = Trig::sin(angles.elbow);
        
        int16_t x = (l1 * cos_s + l2 * cos_e) / 16384;
        int16_t y = (l1 * sin_s + l2 * sin_e) / 16384;
        
        return {x, y, 0};
    }
    
    // Inverse kinematics (simplified 2D)
    JointAngles inverse_kinematics(int16_t x, int16_t y, int16_t l1, int16_t l2) {
        int32_t dist_sq = x*x + y*y;
        int32_t dist = Trig::magnitude(x, y);
        
        // Calculate elbow angle using law of cosines
        int32_t cos_elbow = (dist_sq - l1*l1 - l2*l2) / (2*l1*l2);
        uint16_t elbow = Trig::acos((cos_elbow * 8192) / 16384);
        
        // Calculate shoulder angle
        uint16_t shoulder = Trig::atan2(y, x);
        
        return {shoulder, elbow, 0};
    }
};

// Example 3: Signal processing
class SignalProcessor {
    using Trig = Trig128;
    
public:
    struct Complex {
        int16_t real;
        int16_t imag;
    };
    
    // Rotate complex number by angle (for FFT, modulation, etc.)
    Complex rotate_complex(Complex z, uint16_t angle) {
        int16_t cos_a = Trig::cos(angle);
        int16_t sin_a = Trig::sin(angle);
        
        return {
            static_cast<int16_t>((z.real * cos_a - z.imag * sin_a) / 16384),
            static_cast<int16_t>((z.real * sin_a + z.imag * cos_a) / 16384)
        };
    }
    
    // Generate sine wave samples
    void generate_sine_wave(int16_t* buffer, size_t samples, uint16_t freq_step) {
        uint16_t phase = 0;
        for (size_t i = 0; i < samples; ++i) {
            buffer[i] = Trig::sin(phase);
            phase += freq_step;
        }
    }
    
    // Simple DFT bin calculation
    Complex dft_bin(const int16_t* signal, size_t N, size_t k) {
        int32_t real_sum = 0;
        int32_t imag_sum = 0;
        
        for (size_t n = 0; n < N; ++n) {
            uint16_t angle = (k * n * 16384) / N;
            real_sum += (signal[n] * Trig::cos(angle)) >> 14;
            imag_sum -= (signal[n] * Trig::sin(angle)) >> 14;
        }
        
        return {
            static_cast<int16_t>(real_sum / N),
            static_cast<int16_t>(imag_sum / N)
        };
    }
};

// Example 4: Game physics
class GamePhysics {
    using Trig = Trig64;  // Lower precision acceptable for games
    
public:
    struct Projectile {
        int16_t x, y;
        int16_t vx, vy;
    };
    
    Projectile launch(int16_t speed, uint16_t angle) {
        int16_t vx = (speed * Trig::cos(angle)) / 16384;
        int16_t vy = (speed * Trig::sin(angle)) / 16384;
        
        return {0, 0, vx, vy};
    }
    
    uint16_t get_impact_angle(Projectile p) {
        return Trig::atan2(p.vy, p.vx);
    }
    
    // Collision detection helper
    bool check_angle_between(uint16_t angle, uint16_t start, uint16_t end) {
        // Normalize angles
        uint16_t delta_end = end - start;
        uint16_t delta_angle = angle - start;
        
        return delta_angle <= delta_end;
    }
};

// Example 5: Motor control
class MotorController {
    using Trig = Trig128;
    
    struct MotorVector {
        int16_t d;  // Direct axis
        int16_t q;  // Quadrature axis
    };
    
public:
    // Park transformation for FOC (Field Oriented Control)
    MotorVector park_transform(int16_t alpha, int16_t beta, uint16_t theta) {
        int16_t cos_theta = Trig::cos(theta);
        int16_t sin_theta = Trig::sin(theta);
        
        return {
            static_cast<int16_t>((alpha * cos_theta + beta * sin_theta) >> 14),
            static_cast<int16_t>((-alpha * sin_theta + beta * cos_theta) >> 14)
        };
    }
    
    // Inverse Park transformation
    void inverse_park(MotorVector dq, uint16_t theta, int16_t& alpha, int16_t& beta) {
        int16_t cos_theta = Trig::cos(theta);
        int16_t sin_theta = Trig::sin(theta);
        
        alpha = (dq.d * cos_theta - dq.q * sin_theta) >> 14;
        beta = (dq.d * sin_theta + dq.q * cos_theta) >> 14;
    }
};

// Performance benchmark function
void benchmark() {
    std::cout << "\nPerformance Benchmark:\n";
    std::cout << "=====================\n";
    
    const int iterations = 1000000;
    volatile int16_t result = 0;
    
    // Benchmark sin
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::sin(i & 0x3FFF);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Sin:      " << std::setw(8) << duration.count() << " μs for " 
              << iterations << " ops (" 
              << (duration.count() * 1000.0 / iterations) << " ns/op)\n";
    
    // Benchmark cos
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::cos(i & 0x3FFF);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Cos:      " << std::setw(8) << duration.count() << " μs for " 
              << iterations << " ops (" 
              << (duration.count() * 1000.0 / iterations) << " ns/op)\n";
    
    // Benchmark atan2
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::atan2(i & 0x1FFF, (i >> 4) & 0x1FFF);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Atan2:    " << std::setw(8) << duration.count() << " μs for " 
              << iterations << " ops (" 
              << (duration.count() * 1000.0 / iterations) << " ns/op)\n";
    
    // Benchmark magnitude
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::magnitude(i & 0x1FFF, (i >> 4) & 0x1FFF);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Magnitude:" << std::setw(8) << duration.count() << " μs for " 
              << iterations << " ops (" 
              << (duration.count() * 1000.0 / iterations) << " ns/op)\n";
}

// Main demonstration program
int main() {
    std::cout << "FastTrig Library Examples\n";
    std::cout << "=========================\n\n";
    
    // Memory usage
    std::cout << "Memory Usage:\n";
    std::cout << "Trig32:  " << std::setw(4) << Trig32::table_memory() << " bytes\n";
    std::cout << "Trig64:  " << std::setw(4) << Trig64::table_memory() << " bytes\n";
    std::cout << "Trig128: " << std::setw(4) << Trig128::table_memory() << " bytes (recommended)\n";
    std::cout << "Trig256: " << std::setw(4) << Trig256::table_memory() << " bytes\n\n";
    
    // Accuracy test
    std::cout << "Accuracy Test (Trig128):\n";
    std::cout << std::setw(7) << "Angle" << std::setw(10) << "Sin" 
              << std::setw(10) << "Cos" << std::setw(10) << "Tan\n";
    std::cout << std::string(37, '-') << "\n";
    
    for (int deg = 0; deg <= 360; deg += 30) {
        uint16_t angle = AngleConvert::from_degrees(deg);
        int16_t s = Trig128::sin(angle);
        int16_t c = Trig128::cos(angle);
        
        std::cout << std::setw(5) << deg << "°" 
                  << std::fixed << std::setprecision(4)
                  << std::setw(10) << (s/16384.0 * 2.0)
                  << std::setw(10) << (c/16384.0 * 2.0);
        
        if ((deg % 180 == 90)) {
            std::cout << std::setw(10) << "±∞";
        } else {
            int16_t t = Trig128::tan(angle);
            std::cout << std::setw(10) << (t/8192.0);
        }
        std::cout << "\n";
    }
    
    // Robot navigation example
    std::cout << "\nRobot Navigation Example:\n";
    RobotNavigator nav;
    auto target = nav.calculate_target({0, 0}, {1000, 1000});
    std::cout << "Target heading: " << AngleConvert::to_degrees(target.heading) << "°\n";
    std::cout << "Target distance: " << target.distance << " mm\n";
    
    auto new_pos = nav.move({0, 0}, target.heading, 500);
    std::cout << "After moving 500mm: (" << new_pos.x << ", " << new_pos.y << ")\n";
    
    // Signal processing example
    std::cout << "\nSignal Processing Example:\n";
    SignalProcessor dsp;
    const size_t samples = 64;
    int16_t signal[samples];
    
    // Generate 1/8 sample rate sine wave
    uint16_t freq = 16384 / 8;  // 1/8 of sample rate
    dsp.generate_sine_wave(signal, samples, freq);
    
    std::cout << "Generated " << samples << " samples of sine wave\n";
    std::cout << "First 8 samples: ";
    for (int i = 0; i < 8; ++i) {
        std::cout << signal[i] << " ";
    }
    std::cout << "\n";
    
    // Game physics example
    std::cout << "\nGame Physics Example:\n";
    GamePhysics physics;
    uint16_t launch_angle = AngleConvert::from_degrees(45);
    auto projectile = physics.launch(1000, launch_angle);
    std::cout << "Projectile launched at 45°\n";
    std::cout << "Initial velocity: (" << projectile.vx << ", " << projectile.vy << ")\n";
    
    // Run performance benchmark
    benchmark();
    
    std::cout << "\nAll examples completed successfully!\n";
    return 0;
}
