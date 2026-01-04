// fast_trig.hpp - High-performance integer trigonometry library for embedded systems
// Version: 1.0.0
// Author: Optimized implementation for embedded systems
// License: MIT
//
// Optimized for minimal memory usage and maximum speed
// No floating-point operations, all integer arithmetic
// Memory usage: 384 bytes for 128-entry configuration

#ifndef FAST_TRIG_HPP
#define FAST_TRIG_HPP

#include <cstdint>
#include <array>
#include <concepts>

namespace FastTrig {

// Configuration options
template<std::size_t TableSize = 128>
requires (TableSize >= 8 && TableSize <= 4096 && (TableSize & (TableSize - 1)) == 0)
class IntegerTrig {
public:
    // Constants
    static constexpr uint16_t ANGLE_MAX = 8192;      // 2π in angle units
    static constexpr int16_t OUTPUT_SCALE = 8192;    // ±16384 for ±2.0 output range
    
    // ============================================================
    // Core trigonometric functions
    // ============================================================
    
    // Sine function
    // Input: angle in units where 0-16384 represents 0-2π
    // Output: sine value scaled by ±16384 (±2.0)
    [[nodiscard]] 
    static constexpr int16_t sin(uint16_t angle) noexcept {
        angle &= 0x3FFF;  // Fast modulo using bit mask
        
        uint8_t quadrant = angle >> 12;
        uint16_t position = angle & 0xFFF;
        
        if (quadrant & 1) {
            position = 0x1000 - position;
        }
        
        uint32_t index_scaled = position * RECIPROCAL_QUADRANT;
        uint32_t index = index_scaled >> 16;
        uint8_t fraction = (index_scaled >> 8) & 0xFF;
        
        index = (index < TableSize - 1) ? index : (TableSize - 1);
        
        int32_t y0 = sine_quarter_table[index];
        int32_t y1 = sine_quarter_table[(index + 1) & TABLE_MASK];
        
        int16_t value = static_cast<int16_t>(y0 + (((y1 - y0) * fraction) >> 8));
        
        // Branchless conditional negate
        int16_t sign_mask = -(quadrant >> 1);
        return (value ^ sign_mask) - sign_mask;
    }
    
    // Cosine function
    [[nodiscard]] 
    static constexpr int16_t cos(uint16_t angle) noexcept {
        return sin(angle + (ANGLE_MAX >> 2));
    }
    
    // Tangent function
    [[nodiscard]] 
    static int16_t tan(uint16_t angle) noexcept {
        int16_t sin_val = sin(angle);
        int16_t cos_val = cos(angle);
        
        if (cos_val > -100 && cos_val < 100) {
            return (sin_val >= 0) ? 32767 : -32767;
        }
        
        int32_t result = (int32_t(sin_val) * OUTPUT_SCALE) / cos_val;
        
        if (result > 32767) return 32767;
        if (result < -32767) return -32767;
        
        return static_cast<int16_t>(result);
    }
    
    // ============================================================
    // Inverse trigonometric functions
    // ============================================================
    
    // Arctangent of y/x, returns angle in standard units
    [[nodiscard]] 
    static uint16_t atan2(int16_t y, int16_t x) noexcept {
        if (x == 0) {
            if (y > 0) return ANGLE_MAX >> 1;
            if (y < 0) return (ANGLE_MAX >> 1) * 3;
            return 0;
        }
        
        uint32_t abs_x = (x < 0) ? -x : x;
        uint32_t abs_y = (y < 0) ? -y : y;
        uint8_t quadrant_adjust = ((x < 0) << 1) | (y < 0);
        
        uint16_t angle;
        
        if (abs_x >= abs_y) {
            uint32_t ratio_scaled = (abs_y << TABLE_BITS) / abs_x;
            uint32_t index = ratio_scaled & TABLE_MASK;
            uint32_t fraction = ((abs_y << (TABLE_BITS + 8)) / abs_x) & 0xFF;
            
            int32_t y0 = atan_quarter_table[index];
            int32_t y1 = atan_quarter_table[(index + 1) & TABLE_MASK];
            
            angle = static_cast<uint16_t>(y0 + (((y1 - y0) * fraction) >> 8));
        } else {
            uint32_t ratio_scaled = (abs_x << TABLE_BITS) / abs_y;
            uint32_t index = ratio_scaled & TABLE_MASK;
            uint32_t fraction = ((abs_x << (TABLE_BITS + 8)) / abs_y) & 0xFF;
            
            int32_t y0 = atan_quarter_table[index];
            int32_t y1 = atan_quarter_table[(index + 1) & TABLE_MASK];
            
            uint16_t base = static_cast<uint16_t>(y0 + (((y1 - y0) * fraction) >> 8));
            angle = (ANGLE_MAX >> 1) - base;
        }
        
        static constexpr uint16_t quadrant_offset[4] = {
            0, 2 * ANGLE_MAX, ANGLE_MAX, ANGLE_MAX
        };
        
        static constexpr int16_t angle_sign[4] = {
            1, -1, -1, 1
        };
        
        return quadrant_offset[quadrant_adjust] + (angle * angle_sign[quadrant_adjust]);
    }
    
    // Single-argument arctangent
    [[nodiscard]] 
    static uint16_t atan(int16_t value) noexcept {
        return atan2(value, OUTPUT_SCALE * 2);
    }
    
    // Arcsine function
    // Input: value scaled by ±8192 for ±1.0
    [[nodiscard]] 
    static uint16_t asin(int16_t value) noexcept {
        uint32_t abs_val = (value < 0) ? -value : value;
        abs_val = (abs_val > OUTPUT_SCALE * 2) ? OUTPUT_SCALE * 2 : abs_val;
        
        constexpr uint32_t ASIN_RECIPROCAL = (TableSize << 16) / (OUTPUT_SCALE * 2);
        
        uint32_t index_scaled = abs_val * ASIN_RECIPROCAL;
        uint32_t index = index_scaled >> 16;
        uint8_t fraction = (index_scaled >> 8) & 0xFF;
        
        index = (index < TableSize - 1) ? index : (TableSize - 1);
        
        int32_t y0 = asin_quarter_table[index];
        int32_t y1 = asin_quarter_table[(index + 1) & TABLE_MASK];
        
        uint16_t angle = static_cast<uint16_t>(y0 + (((y1 - y0) * fraction) >> 8));
        
        return (value < 0) ? (2 * ANGLE_MAX - angle) : angle;
    }
    
    // Arccosine function
    [[nodiscard]] 
    static uint16_t acos(int16_t value) noexcept {
        uint16_t asin_result = asin(value);
        return (ANGLE_MAX >> 1) - asin_result;
    }
    
    // ============================================================
    // Utility functions
    // ============================================================
    
    // CORDIC magnitude calculation (Pythagorean distance)
    [[nodiscard]] 
    static int32_t magnitude(int32_t x, int32_t y) noexcept {
        uint32_t abs_x = (x < 0) ? -x : x;
        uint32_t abs_y = (y < 0) ? -y : y;
        
        for (int i = 0; i < 12; ++i) {
            uint32_t x_shift = abs_x >> i;
            uint32_t y_shift = abs_y >> i;
            
            if (abs_y > 0) {
                uint32_t x_new = abs_x + y_shift;
                uint32_t y_new = (abs_y > x_shift) ? abs_y - x_shift : 0;
                abs_x = x_new;
                abs_y = y_new;
            }
        }
        
        return (abs_x * 39797) >> 16;
    }
    
    // Simultaneous sine and cosine calculation
    static void sincos(uint16_t angle, int16_t& sin_out, int16_t& cos_out) noexcept {
        sin_out = sin(angle);
        cos_out = cos(angle);
    }
    
    // Get memory usage information
    static constexpr std::size_t table_memory() { 
        return sizeof(sine_quarter_table) + sizeof(atan_quarter_table) + sizeof(asin_quarter_table); 
    }
    
    static constexpr std::size_t table_size() { return TableSize; }

private:
    // Precomputed constants for optimization
    static constexpr int TABLE_BITS = __builtin_ctz(TableSize);
    static constexpr uint32_t TABLE_MASK = TableSize - 1;
    static constexpr uint32_t RECIPROCAL_QUADRANT = (TableSize << 16) / 4096;
    
    // Table generation for sine (quarter wave)
    static constexpr int16_t sin_internal(uint32_t angle) {
        uint32_t x = angle;
        uint32_t term = (x * (16384 - x)) >> 14;
        int32_t numerator = term << 2;
        int32_t denominator = 16487 - term;
        if (denominator == 0) return OUTPUT_SCALE * 2;
        return static_cast<int16_t>((numerator * OUTPUT_SCALE * 2) / denominator);
    }
    
    static constexpr std::array<int16_t, TableSize> generate_sine_quarter_table() {
        std::array<int16_t, TableSize> table{};
        for (std::size_t i = 0; i < TableSize; ++i) {
            uint32_t angle = (i * 16384) / (TableSize - 1);
            table[i] = sin_internal(angle);
        }
        return table;
    }
    
    // Table generation for atan (quarter range)
    static constexpr std::array<uint16_t, TableSize> generate_atan_quarter_table() {
        std::array<uint16_t, TableSize> table{};
        for (std::size_t i = 0; i < TableSize; ++i) {
            int32_t x = 16384;
            int32_t y = 0;
            int32_t target_y = (i * 16384) / TableSize;
            uint32_t angle = 0;
            
            for (int k = 0; k < 16; ++k) {
                int32_t x_new, y_new;
                uint32_t angle_step = (2048 >> k);
                
                if (y < target_y) {
                    x_new = x - (y >> k);
                    y_new = y + (x >> k);
                    angle += angle_step;
                } else {
                    x_new = x + (y >> k);
                    y_new = y - (x >> k);
                    angle -= angle_step;
                }
                x = x_new;
                y = y_new;
            }
            
            table[i] = static_cast<uint16_t>(angle);
        }
        return table;
    }
    
    // Table generation for asin (quarter range)
    static constexpr std::array<uint16_t, TableSize> generate_asin_quarter_table() {
        std::array<uint16_t, TableSize> table{};
        for (std::size_t i = 0; i < TableSize; ++i) {
            int32_t target = (i * OUTPUT_SCALE * 2) / TableSize;
            uint32_t low = 0;
            uint32_t high = ANGLE_MAX / 2;
            
            while (high - low > 1) {
                uint32_t mid = (low + high) / 2;
                int32_t sin_mid = sin_internal(mid);
                
                if (sin_mid < target) {
                    low = mid;
                } else {
                    high = mid;
                }
            }
            
            table[i] = static_cast<uint16_t>((low + high) / 2);
        }
        return table;
    }
    
    // Lookup tables (using quarter-wave/range symmetry)
    alignas(64) static constexpr auto sine_quarter_table = generate_sine_quarter_table();
    alignas(64) static constexpr auto atan_quarter_table = generate_atan_quarter_table();
    alignas(64) static constexpr auto asin_quarter_table = generate_asin_quarter_table();
};

// Convenient type aliases for common configurations
using Trig32 = IntegerTrig<32>;    // 96 bytes - Ultra compact
using Trig64 = IntegerTrig<64>;    // 192 bytes - Compact
using Trig128 = IntegerTrig<128>;  // 384 bytes - Balanced (recommended)
using Trig256 = IntegerTrig<256>;  // 768 bytes - High precision
using Trig512 = IntegerTrig<512>;  // 1536 bytes - Very high precision

// Default configuration
using Trig = Trig128;

// ============================================================
// Helper classes for common operations
// ============================================================

template<typename TrigImpl = Trig>
class Vector2D {
public:
    struct Vec2 {
        int16_t x, y;
    };
    
    struct Polar {
        uint16_t angle;
        int16_t magnitude;
    };
    
    [[nodiscard]] static Polar to_polar(const Vec2& v) noexcept {
        return {
            TrigImpl::atan2(v.y, v.x),
            static_cast<int16_t>(TrigImpl::magnitude(v.x, v.y))
        };
    }
    
    [[nodiscard]] static Vec2 from_polar(const Polar& p) noexcept {
        return {
            static_cast<int16_t>((int32_t(p.magnitude) * TrigImpl::cos(p.angle)) >> 14),
            static_cast<int16_t>((int32_t(p.magnitude) * TrigImpl::sin(p.angle)) >> 14)
        };
    }
    
    [[nodiscard]] static Vec2 rotate(const Vec2& v, uint16_t angle) noexcept {
        int16_t cos_a = TrigImpl::cos(angle);
        int16_t sin_a = TrigImpl::sin(angle);
        
        return {
            static_cast<int16_t>((int32_t(v.x) * cos_a - int32_t(v.y) * sin_a) >> 14),
            static_cast<int16_t>((int32_t(v.x) * sin_a + int32_t(v.y) * cos_a) >> 14)
        };
    }
};

// Angle conversion utilities
class AngleConvert {
public:
    // Convert degrees to internal angle units
    [[nodiscard]] static constexpr uint16_t from_degrees(int16_t degrees) noexcept {
        // Handle negative degrees
        while (degrees < 0) degrees += 360;
        while (degrees >= 360) degrees -= 360;
        
        // Convert: 360° = 16384 units
        return (static_cast<uint32_t>(degrees) * 16384) / 360;
    }
    
    // Convert radians (fixed-point) to internal angle units
    // Input: radians * 1000 (e.g., 3141 = 3.141 radians)
    [[nodiscard]] static constexpr uint16_t from_milliradians(int32_t mrad) noexcept {
        // 2π radians = 16384 units
        // 6283 milliradians = 16384 units
        return (static_cast<uint32_t>(mrad) * 16384) / 6283;
    }
    
    // Convert internal units to degrees
    [[nodiscard]] static constexpr int16_t to_degrees(uint16_t angle) noexcept {
        return (static_cast<uint32_t>(angle) * 360) / 16384;
    }
};

} // namespace FastTrig

#endif // FAST_TRIG_HPP
