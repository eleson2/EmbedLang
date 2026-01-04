// test_fast_trig.cpp - Unit tests for FastTrig library

#include "fast_trig.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace FastTrig;

// Helper function to compare with tolerance
bool approx_equal(double a, double b, double tolerance = 0.01) {
    return std::abs(a - b) < tolerance;
}

// Test accuracy against standard library
void test_accuracy() {
    std::cout << "Testing accuracy against standard library...\n";
    
    double max_sin_error = 0;
    double max_cos_error = 0;
    double total_sin_error = 0;
    double total_cos_error = 0;
    int test_count = 0;
    
    for (int i = 0; i < 16384; i += 10) {  // Test every 10th angle
        double angle_rad = (2.0 * M_PI * i) / 16384.0;
        
        int16_t fast_sin = Trig128::sin(i);
        int16_t fast_cos = Trig128::cos(i);
        
        double expected_sin = std::sin(angle_rad);
        double expected_cos = std::cos(angle_rad);
        
        double actual_sin = fast_sin / 16384.0;
        double actual_cos = fast_cos / 16384.0;
        
        double sin_error = std::abs(actual_sin - expected_sin);
        double cos_error = std::abs(actual_cos - expected_cos);
        
        max_sin_error = std::max(max_sin_error, sin_error);
        max_cos_error = std::max(max_cos_error, cos_error);
        total_sin_error += sin_error;
        total_cos_error += cos_error;
        test_count++;
    }
    
    std::cout << "  Max sin error: " << std::fixed << std::setprecision(6) 
              << max_sin_error << " (" 
              << max_sin_error * 100 << "%)\n";
    std::cout << "  Max cos error: " << max_cos_error << " (" 
              << max_cos_error * 100 << "%)\n";
    std::cout << "  Avg sin error: " << (total_sin_error / test_count) << "\n";
    std::cout << "  Avg cos error: " << (total_cos_error / test_count) << "\n";
    
    assert(max_sin_error < 0.001);  // Less than 0.1% error
    assert(max_cos_error < 0.001);
    
    std::cout << "  ✓ Accuracy test passed\n\n";
}

// Test atan2 for all quadrants
void test_atan2() {
    std::cout << "Testing atan2 quadrants...\n";
    
    struct TestCase {
        int16_t x, y;
        int16_t expected_degrees;
        const char* description;
    };
    
    std::vector<TestCase> test_cases = {
        {1000, 0, 0, "Positive X axis"},
        {1000, 1000, 45, "First quadrant"},
        {0, 1000, 90, "Positive Y axis"},
        {-1000, 1000, 135, "Second quadrant"},
        {-1000, 0, 180, "Negative X axis"},
        {-1000, -1000, 225, "Third quadrant"},
        {0, -1000, 270, "Negative Y axis"},
        {1000, -1000, 315, "Fourth quadrant"},
        {0, 0, 0, "Origin (undefined)"},
    };
    
    for (const auto& test : test_cases) {
        uint16_t angle = Trig128::atan2(test.y, test.x);
        int16_t degrees = AngleConvert::to_degrees(angle);
        
        // Handle 360° wrap-around
        if (degrees == 360) degrees = 0;
        
        std::cout << "  " << std::setw(20) << test.description 
                  << ": atan2(" << std::setw(5) << test.y 
                  << ", " << std::setw(5) << test.x 
                  << ") = " << std::setw(3) << degrees << "°";
        
        if (std::abs(degrees - test.expected_degrees) <= 1) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗ (expected " << test.expected_degrees << "°)\n";
            assert(false);
        }
    }
    
    std::cout << "  ✓ All atan2 tests passed\n\n";
}

// Test inverse functions
void test_inverse() {
    std::cout << "Testing inverse trigonometric functions...\n";
    
    // Test asin/acos round-trip
    std::cout << "  Testing asin/sin round-trip...\n";
    int errors = 0;
    for (int i = -8192; i <= 8192; i += 500) {
        uint16_t asin_val = Trig128::asin(i);
        int16_t sin_back = Trig128::sin(asin_val);
        
        int error = std::abs(sin_back - i);
        if (error > 100) {  // Allow small error due to quantization
            errors++;
            if (errors < 5) {  // Show first few errors
                std::cout << "    Value: " << i 
                          << " -> asin: " << asin_val 
                          << " -> sin: " << sin_back 
                          << " (error: " << error << ")\n";
            }
        }
    }
    
    if (errors == 0) {
        std::cout << "    ✓ No significant errors\n";
    } else {
        std::cout << "    Total errors: " << errors << "\n";
    }
    
    // Test identity: asin(x) + acos(x) = π/2
    std::cout << "  Testing asin(x) + acos(x) = π/2...\n";
    errors = 0;
    for (int i = -8192; i <= 8192; i += 500) {
        uint16_t asin_val = Trig128::asin(i);
        uint16_t acos_val = Trig128::acos(i);
        
        // Should sum to π/2 (4096 in our units)
        int sum = asin_val + acos_val;
        int error = std::abs(sum - 4096);
        
        if (error > 10) {
            errors++;
            if (errors < 5) {
                std::cout << "    Value: " << i 
                          << " -> asin + acos: " << sum 
                          << " (expected 4096, error: " << error << ")\n";
            }
        }
    }
    
    if (errors == 0) {
        std::cout << "    ✓ Identity holds within tolerance\n";
    } else {
        std::cout << "    Errors: " << errors << "\n";
    }
    
    std::cout << "  ✓ Inverse function tests completed\n\n";
}

// Test magnitude calculation
void test_magnitude() {
    std::cout << "Testing magnitude (CORDIC)...\n";
    
    struct TestCase {
        int32_t x, y;
        int32_t expected;
        const char* description;
    };
    
    std::vector<TestCase> test_cases = {
        {3000, 4000, 5000, "3-4-5 triangle"},
        {5000, 12000, 13000, "5-12-13 triangle"},
        {8000, 15000, 17000, "8-15-17 triangle"},
        {1000, 0, 1000, "Horizontal"},
        {0, 1000, 1000, "Vertical"},
        {1000, 1000, 1414, "45 degrees"},
        {-3000, -4000, 5000, "Negative values"},
    };
    
    for (const auto& test : test_cases) {
        int32_t result = Trig128::magnitude(test.x, test.y);
        double expected = std::sqrt(test.x * test.x + test.y * test.y);
        double error = std::abs(result - expected) / expected;
        
        std::cout << "  " << std::setw(20) << test.description 
                  << ": magnitude(" << test.x << ", " << test.y 
                  << ") = " << result 
                  << " (expected ≈" << static_cast<int>(expected) 
                  << ", error: " << std::fixed << std::setprecision(2) 
                  << (error * 100) << "%)";
        
        if (error < 0.01) {  // Less than 1% error
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗\n";
            assert(false);
        }
    }
    
    std::cout << "  ✓ All magnitude tests passed\n\n";
}

// Test special angle values
void test_special_angles() {
    std::cout << "Testing special angle values...\n";
    
    struct AngleTest {
        int degrees;
        double expected_sin;
        double expected_cos;
    };
    
    std::vector<AngleTest> special_angles = {
        {0, 0.0, 1.0},
        {30, 0.5, 0.866},
        {45, 0.707, 0.707},
        {60, 0.866, 0.5},
        {90, 1.0, 0.0},
        {120, 0.866, -0.5},
        {135, 0.707, -0.707},
        {150, 0.5, -0.866},
        {180, 0.0, -1.0},
        {210, -0.5, -0.866},
        {225, -0.707, -0.707},
        {240, -0.866, -0.5},
        {270, -1.0, 0.0},
        {300, -0.866, 0.5},
        {315, -0.707, 0.707},
        {330, -0.5, 0.866},
        {360, 0.0, 1.0},
    };
    
    for (const auto& test : special_angles) {
        uint16_t angle = AngleConvert::from_degrees(test.degrees);
        double sin_val = Trig128::sin(angle) / 16384.0 * 2.0;
        double cos_val = Trig128::cos(angle) / 16384.0 * 2.0;
        
        std::cout << "  " << std::setw(3) << test.degrees << "°: "
                  << "sin=" << std::fixed << std::setprecision(3) 
                  << std::setw(7) << sin_val 
                  << " (expected " << std::setw(7) << test.expected_sin << "), "
                  << "cos=" << std::setw(7) << cos_val 
                  << " (expected " << std::setw(7) << test.expected_cos << ")";
        
        if (approx_equal(sin_val, test.expected_sin, 0.01) && 
            approx_equal(cos_val, test.expected_cos, 0.01)) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗\n";
            assert(false);
        }
    }
    
    std::cout << "  ✓ All special angle tests passed\n\n";
}

// Test different table sizes
void test_table_sizes() {
    std::cout << "Testing different table sizes...\n";
    
    // Test a few angles with different precisions
    uint16_t test_angle = AngleConvert::from_degrees(30);
    
    std::cout << "  Sin(30°) with different table sizes:\n";
    
    int16_t sin32 = Trig32::sin(test_angle);
    int16_t sin64 = Trig64::sin(test_angle);
    int16_t sin128 = Trig128::sin(test_angle);
    int16_t sin256 = Trig256::sin(test_angle);
    
    double expected = 0.5;
    
    std::cout << "    Trig32:  " << (sin32 / 16384.0 * 2.0) 
              << " (error: " << std::abs((sin32 / 16384.0 * 2.0) - expected) << ")\n";
    std::cout << "    Trig64:  " << (sin64 / 16384.0 * 2.0) 
              << " (error: " << std::abs((sin64 / 16384.0 * 2.0) - expected) << ")\n";
    std::cout << "    Trig128: " << (sin128 / 16384.0 * 2.0) 
              << " (error: " << std::abs((sin128 / 16384.0 * 2.0) - expected) << ")\n";
    std::cout << "    Trig256: " << (sin256 / 16384.0 * 2.0) 
              << " (error: " << std::abs((sin256 / 16384.0 * 2.0) - expected) << ")\n";
    
    // Verify accuracy improves with table size
    double error32 = std::abs((sin32 / 16384.0 * 2.0) - expected);
    double error64 = std::abs((sin64 / 16384.0 * 2.0) - expected);
    double error128 = std::abs((sin128 / 16384.0 * 2.0) - expected);
    double error256 = std::abs((sin256 / 16384.0 * 2.0) - expected);
    
    assert(error256 <= error128);
    assert(error128 <= error64);
    assert(error64 <= error32);
    
    std::cout << "  ✓ Accuracy improves with table size\n\n";
}

// Test sincos simultaneous calculation
void test_sincos() {
    std::cout << "Testing simultaneous sin/cos calculation...\n";
    
    for (int deg = 0; deg <= 360; deg += 45) {
        uint16_t angle = AngleConvert::from_degrees(deg);
        
        int16_t sin_separate = Trig128::sin(angle);
        int16_t cos_separate = Trig128::cos(angle);
        
        int16_t sin_simul, cos_simul;
        Trig128::sincos(angle, sin_simul, cos_simul);
        
        std::cout << "  " << std::setw(3) << deg << "°: ";
        
        if (sin_separate == sin_simul && cos_separate == cos_simul) {
            std::cout << "✓ Values match\n";
        } else {
            std::cout << "✗ Mismatch!\n";
            std::cout << "    Separate: sin=" << sin_separate 
                      << ", cos=" << cos_separate << "\n";
            std::cout << "    Simultaneous: sin=" << sin_simul 
                      << ", cos=" << cos_simul << "\n";
            assert(false);
        }
    }
    
    std::cout << "  ✓ sincos test passed\n\n";
}

// Main test runner
int main() {
    std::cout << "FastTrig Library Test Suite\n";
    std::cout << "===========================\n\n";
    
    try {
        test_accuracy();
        test_atan2();
        test_inverse();
        test_magnitude();
        test_special_angles();
        test_table_sizes();
        test_sincos();
        
        std::cout << "=============================\n";
        std::cout << "✓ All tests passed!\n";
        std::cout << "=============================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
