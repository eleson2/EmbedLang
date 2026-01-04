// test_fast_trig.cpp - Unit tests for FastTrig library

#include "fast_trig.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace FastTrig;

// Test accuracy against standard library
void test_accuracy() {
    std::cout << "Testing accuracy...\n";
    
    double max_sin_error = 0;
    double max_cos_error = 0;
    
    for (int i = 0; i < 16384; ++i) {
        double angle_rad = (2.0 * M_PI * i) / 16384.0;
        
        int16_t fast_sin = Trig128::sin(i);
        int16_t fast_cos = Trig128::cos(i);
        
        double expected_sin = std::sin(angle_rad) * 2.0;
        double expected_cos = std::cos(angle_rad) * 2.0;
        
        double actual_sin = fast_sin / 16384.0;
        double actual_cos = fast_cos / 16384.0;
        
        double sin_error = std::abs(actual_sin - expected_sin);
        double cos_error = std::abs(actual_cos - expected_cos);
        
        max_sin_error = std::max(max_sin_error, sin_error);
        max_cos_error = std::max(max_cos_error, cos_error);
    }
    
    std::cout << "Max sin error: " << max_sin_error << " (" 
              << max_sin_error * 100 / 2.0 << "%)\n";
    std::cout << "Max cos error: " << max_cos_error << " (" 
              << max_cos_error * 100 / 2.0 << "%)\n";
    
    assert(max_sin_error < 0.001);  // Less than 0.1% error
    assert(max_cos_error < 0.001);
}

// Test atan2 quadrants
void test_atan2() {
    std::cout << "Testing atan2...\n";
    
    // Test all quadrants
    assert(AngleConvert::to_degrees(Trig128::atan2(1000, 1000)) == 45);    // Q1
    assert(AngleConvert::to_degrees(Trig128::atan2(1000, -1000)) == 135);  // Q2
    assert(AngleConvert::to_degrees(Trig128::atan2(-1000, -1000)) == 225); // Q3
    assert(AngleConvert::to_degrees(Trig128::atan2(-1000, 1000)) == 315);  // Q4
    
    // Test special cases
    assert(AngleConvert::to_degrees(Trig128::atan2(1000, 0)) == 90);
    assert(AngleConvert::to_degrees(Trig128::atan2(-1000, 0)) == 270);
    assert(AngleConvert::to_degrees(Trig128::atan2(0, 1000)) == 0);
    assert(AngleConvert::to_degrees(Trig128::atan2(0, -1000)) == 180);
    
    std::cout << "atan2 tests passed\n";
}

// Test inverse functions
void test_inverse() {
    std::cout << "Testing inverse functions...\n";
    
    // Test asin/acos
    for (int i = -8192; i <= 8192; i += 1000) {
        uint16_t asin_val = Trig128::asin(i);
        int16_t sin_back = Trig128::sin(asin_val);
        
        // Allow small error due to quantization
        assert(std::abs(sin_back - i) < 100);
    }
    
    // Test identity: asin(x) + acos(x) = π/2
    for (int i = -8192; i <= 8192; i += 1000) {
        uint16_t asin_val = Trig128::asin(i);
        uint16_t acos_val = Trig128::acos(i);
        
        // Should sum to π/2 (4096 in our units)
        assert(std::abs(int(asin_val + acos_val) - 4096) < 10);
    }
    
    std::cout << "Inverse function tests passed\n";
}

// Performance benchmark
void benchmark() {
    std::cout << "\nPerformance Benchmark:\n";
    
    const int iterations = 1000000;
    volatile int16_t result = 0;
    
    // Benchmark sin/cos
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::sin(i & 0x3FFF);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Sin: " << duration.count() << " us for " << iterations 
              << " ops (" << (duration.count() * 1000.0 / iterations) 
              << " ns/op)\n";
    
    // Benchmark atan2
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = Trig128::atan2(i & 0x1FFF, (i >> 4) & 0x1FFF);
    }
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Atan2: " << duration.count() << " us for " << iterations 
              << " ops (" << (duration.count() * 1000.0 / iterations) 
              << " ns/op)\n";
}

int main() {
    std::cout << "FastTrig Library Test Suite\n";
    std::cout << "===========================\n\n";
    
    test_accuracy();
    test_atan2();
    test_inverse();
    benchmark();
    
    std::cout << "\nAll tests passed!\n";
    return 0;
}