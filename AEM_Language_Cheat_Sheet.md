This AEM Language Cheat Sheet serves as the primary reference for both the human architect and the AI developer. It encapsulates every decision we’ve made to ensure zero-overhead, integer-only, bare-metal performance.

# AEM Language Cheat Sheet (Draft 1.0)

## 1. Core Philosophy
*   **AI-First**: Explicit, predictable syntax that minimizes hallucination risks.
*   **No Pre-processor**: All configuration is handled via first-class language constructs.
*   **Zero-Overhead**: Transpiles to optimized C++ using static polymorphism (no V-tables).
*   **Deterministic**: Fixed-width types, static allocation, and non-preemptive scheduling.
*   **Integer-Only**: All calculations use integer math, favoring fixed-point arithmetic and bit-shifts.

## 2. Type System & Math
AEM uses strictly defined integer types to ensure cross-platform consistency and efficient bare-metal execution.

| Type             | Description                 | C++ Mapping               |
| :--------------- | :-------------------------- | :------------------------ |
| `u8`, `u16`, `u32`, `u64` | Unsigned Integers           | `uint8_t`, `uint16_t`, ... |
| `i8`, `i16`, `i32`, `i64` | Signed Integers             | `int8_t`, `int16_t`, ...   |
| `bool`           | Boolean (`true`/`false`)    | `bool`                    |
| `void`           | No return value             | `void`                    |

**Fixed-Point Rule**: Decimal values must be scaled to integers (e.g., `24.50` -> `2450`). Division by powers of 2 (via bit-shifting) is preferred for performance. Always cast to a larger type (e.g., `i32`) before multiplication to prevent overflow.

**Casting**: Explicit casting using the `as` keyword is mandatory (`value as u16`). There is no implicit type promotion. Narrowing casts (`u16` to `u8`) require careful handling to prevent data loss (e.g., using `match` for clamping).

**Bitwise Operators**: `&`, `|`, `^`, `~`, `<<`, `>>` are available for bit manipulation.

## 3. Keywords & Program Structure

| Keyword       | Purpose                                                                                                                              | Example                                        |
| :------------ | :----------------------------------------------------------------------------------------------------------------------------------- | :--------------------------------------------- |
| `hardware`    | Global block for defining physical I/O pins, devices, and interrupt bindings.                                                        | `hardware { pin LED at 13 [output]; }`         |
| `task`        | Defines a non-preemptive, scheduled function.                                                                                        | `task BlinkLED { LED.toggle(); }`              |
| `interface`   | Defines a static contract (blueprint) for behavior that `struct`s can implement. No internal state for pure interfaces.             | `interface Motor { fn setSpeed(u8: speed); }`  |
| `struct`      | Defines a data structure. Can be generic and implement `interface`s.                                                                 | `struct Point<T> { let x: T; let y: T; }`      |
| `queue`       | Declares a statically allocated, single-writer, single-reader buffer.                                                                | `queue Readings <i16, 16> [overwrite_old];`    |
| `fn`          | Defines a function. Supports hoisting.                                                                                               | `fn calculate(i16: val) : i16 { return val; }` |
| `const fn`    | Defines a function that can be evaluated at compile-time. Transpiles to `constexpr`.                                               | `const fn pow(f32: b, u8: e) : f32 { ... }`   |
| `let`/`let mut` | Declares an immutable (`let`) or mutable (`let mut`) variable or constant.                                                           | `let count: u8 = 0; let mut state: bool = false;` |
| `const`       | Declares a compile-time constant, for single values or arrays. | `const MAX_VAL: u16 = 255;` `const LUT: [u8; 4] = [0, 1, 2, 3];` |
| `module`      | Defines a namespace. Can be generic over `const` parameters.                                                                         | `use trig<16>;`                                |
| `use`         | Imports modules or specific symbols into the current scope.                                                                          | `use my_module::MyStruct;`                     |
| `return`      | Returns a value from a function.                                                                                                     | `return true;`                                 |
| `self`        | Refers to the current `struct` instance within a method.                                                                             | `self.field = 5;`                              |
| `if`/`else if`/`else` | Conditional branching. Mandatory braces `{}`.                                                                                | `if (x > 0) { ... } else { ... }`              |
| `loop`        | Infinite loop.                                                                                                                       | `loop { ... }`                                 |
| `for`         | Range-based loop.                                                                                                                    | `for i in 0..10 { ... }`                       |
| `while`       | Conditional loop.                                                                                                                    | `while (condition) { ... }`                    |
| `break`/`continue` | Control loop execution. Supports labeled breaks.                                                                                     | `loop 'outer { ... if (x) break 'outer; }`   |
| `match`       | Exhaustive pattern matching. Replaces C++ `switch`. No fall-through.                                                                | `match x { 0..10 => { ... }, _ => { ... } }`   |
| `as`          | Explicit type casting.                                                                                                               | `(my_i8 as u16) + (my_u8 as u16)`              |

### Built-in Functions
AEM provides a few intrinsic functions that directly map to low-level hardware access or compiler features. These do not require a `use` statement.

*   `volatile_read<T>(address: u32) : T`
    *   **Purpose**: Reads a value of type `T` from a specific memory `address`.
    *   **Usage**: Used for direct interaction with memory-mapped hardware registers. The `volatile` nature prevents the compiler from optimizing away the read.
    *   **Transpiles to**: `*(volatile T*)address` in C++.
    *   **Example**: `let reg_value = volatile_read<u32>(0x40021000);`

*   `volatile_write<T>(address: u32, value: T)`
    *   **Purpose**: Writes a `value` of type `T` to a specific memory `address`.
    *   **Usage**: Used for direct interaction with memory-mapped hardware registers. The `volatile` nature prevents the compiler from optimizing away the write.
    *   **Transpiles to**: `*(volatile T*)address = value;` in C++.
    *   **Example**: `volatile_write<u32>(0x40021000, new_value);`

## 4. Hardware Manifest (`hardware` block)
Defines static mappings of logical names to physical hardware.

```aem
hardware {
    pin StatusLED at 13 [output, active_high]; // Digital output pin
    pin UserBtn   at 2  [input, pull_up];       // Digital input pin with pull-up
    device TempSensor at I2C_0 [address: 0x48]; // I2C device at specific address

    // Static Interrupt Binding
    interrupt OnClick on UserBtn [falling] calls HandleButton; 
}
```

## 5. Tasks & Scheduling
Tasks are declared with the `task` keyword and managed procedurally.

```aem
task BlinkLED {
    // Task logic
    LED.toggle();
}

fn main_setup() {
    BlinkLED.setInterval(500ms);  // Set interval (time literals: ms, us, s)
    BlinkLED.start();             // Start the task
    BlinkLED.stop();              // Stop the task
    BlinkLED.startNow();          // Run immediately once
    BlinkLED.startLater();        // Run on the next scheduler tick
}
```

## 6. Queues
Statically allocated, single-writer, single-reader, lock-free queues for inter-task/ISR communication. Capacity must be a power of 2.

```aem
// queue Name <Type, Capacity> [Policy];
queue SensorBuffer <i16, 4> [overwrite_old]; // Discards oldest if full
// queue AnotherQueue <u8, 8> [discard_new]; // Discards new if full

fn DataProducer() {
    let data = ReadSensor();
    SensorBuffer.push(data); // Non-blocking
}

fn DataConsumer() {
    let mut current_val: i16 = 0;
    if SensorBuffer.try_fetch(current_val) { // Non-blocking read
        // Process current_val
    }
}
```

## 7. Interfaces & Structs
AEM uses static interfaces to enforce contracts without V-table overhead. Structs implement interfaces explicitly.

```aem
interface DigitalIO {
    fn set(bool: level);
    fn toggle();
    fn read() : bool;
    fn set_mode(u8: mode); // mode constants: INPUT, OUTPUT, PULLUP, ANALOG
}

struct MyPin : DigitalIO { // MyPin explicitly implements DigitalIO
    let pin_number: u8;

    fn set(bool: level) { /* Low-level register write */ }
    fn toggle() { /* ... */ }
    fn read() : bool { /* ... */ }
    fn set_mode(u8: mode) { /* ... */ }
}

fn operate_pin(DigitalIO: pin_instance) {
    pin_instance.set(true);
}
```

## 8. Generics & Compile-Time Execution
AEM supports compile-time customization and code generation through `const` generic parameters and `const fn`.

### Const Generic Parameters
Structs and modules can be made generic over constant values, allowing for highly reusable and efficient code.

```aem
// A MovingAverage filter generic over its sample size
struct MovingAverage<const SIZE: u8> {
    let mut samples: [i16; SIZE];
    // ...
}

// A trigonometry module generic over its LUT size
module trig<const ENTRIES: u16> {
    // ...
}

// Usage
let small_filter = MovingAverage<8>();
use trig<32>; // Use trig library specialized for a 32-entry LUT
```

### Compile-Time Functions (`const fn`)
A function marked `const fn` can be evaluated by the C++ compiler during the compilation process. This allows for complex initializations of `const` variables, such as generating a lookup table (LUT) on the fly.

- **Rule**: `const fn` is translated directly to a C++ `constexpr` function.
- **Capability**: Enables compile-time generation of data, eliminating the need for pre-calculated tables in source code.

```aem
const fn factorial(u8: n) : u32 {
    if (n == 0) { return 1; }
    return n as u32 * factorial(n - 1);
}

// Use the const fn to initialize a const variable
const COMPILE_TIME_VALUE: u32 = factorial(5); // Value is 120
```

## 9. Error Handling
AEM uses the "Result Pattern" instead of exceptions. Functions that can fail return a value that must be explicitly checked. A `!` suffix on a type can indicate a fallible return.

```aem
fn ReadSensor() : i16! { // Can return an i16 or an error state
    if Hardware.is_ready() {
        return Hardware.read();
    }
    return Error; // Transpiler generates appropriate error state
}

fn process_data() {
    let result = ReadSensor();
    if result.is_error() {
        // Handle error
    } else {
        let value = result.get_value(); // Get the actual value
        // Process value
    }
}
```
## 10. Standard Library Modules

### `Clock` Module
*   `Clock.millis() : u32` - Current milliseconds since startup.
*   `Clock.micros() : u32` - Current microseconds since startup.
*   `Clock.delay_ms(u32: ms)` - Blocking delay in milliseconds.
*   `Clock.delay_us(u32: us)` - Blocking delay in microseconds.
*   `Clock.elapsed(u32: start_time, u32: duration) : bool` - Checks if `duration` has passed since `start_time` (handles rollover).

### `GPIO` Interface (for individual pins)
*   `fn set(bool: level)`
*   `fn toggle()`
*   `fn read() : bool`
*   `fn set_mode(u8: mode)` - `mode` constants: `INPUT`, `OUTPUT`, `PULLUP`, `ANALOG`.

### `ADC` Interface
*   `fn read_raw() : u16` - Returns raw ADC reading.
*   `fn read_mv() : u16` - Returns ADC reading scaled to millivolts (e.g., 0-5000).
*   `fn set_reference(u8: ref_source)` - Sets ADC voltage reference.

### `Stream` Interface (e.g., UART, USB)
*   `fn write(u8: byte)`
*   `fn read() : u8`
*   `fn available() : u16`
*   `fn flush()`
*   `fn print(str: msg)` - Transpiler converts to a sequence of `write()` calls.

### `I2C` Interface (for bus communication)
*   `fn write_reg(u8: addr, u8: reg, u8: val)` - Writes a byte `val` to `reg` on device `addr`.
*   `fn read_reg(u8: addr, u8: reg) : u8` - Reads a byte from `reg` on device `addr`.
*   `fn request(u8: addr, u8: len)` - Initiates a multi-byte read from device `addr`.

### `Math` Module (integer-only utilities)
*   `math.map(i32: x, i32: in_min, i32: in_max, i32: out_min, i32: out_max) : i32`
*   `math.clamp(i32: x, i32: min, i32: max) : i32`
*   `math.abs(i32: x) : i32`
*   `math.min(i32: a, i32: b) : i32`
*   `math.max(i32: a, i32: b) : i32`

### `Trig` Module (Generic)
*   A generic, self-generating trigonometry library.
*   `use trig<ENTRIES>;` where `ENTRIES` is the desired LUT size (e.g., 16, 32).
*   The module uses `const fn` to build its own LUT at compile-time.
*   Provides `sin(u8: angle)`, `cos(u8: angle)`, etc.

### Standard Constants (Accessed via `const` or `module::Constant`)
*   `SYS_FREQ` - CPU Frequency in Hz.
*   `BOARD_NAME` - String name of the target board.
*   `TICK_RATE` - Scheduler tick resolution (e.g., 1ms).

## 11. Transpilation to C++ Considerations
*   **Compile-Time Execution**: `const fn` in AEM is transpiled to `constexpr` in C++, offloading compile-time calculations to the C++ compiler.
*   **Generics**: Generic modules and structs are transpiled using C++ templates, creating specialized versions of the code at compile-time (monomorphization).
*   **Static Polymorphism**: Interfaces map to C++ templates or concepts, avoiding runtime overhead.
*   **Queues**: Map to specialized, fixed-size C++ ring buffer classes optimized for SPSC.
*   **Tasks**: Map to C++ objects registered with a non-preemptive scheduler (e.g., Arkipenko's).
*   **Integer Math**: AEM bit-shifts map directly to C++ bit-shifts.
*   **Error Handling**: AEM `Result` pattern maps to `std::optional` (if available and no overhead) or custom error `struct`s.
*   **Interrupts**: `hardware` block definitions generate specific C++ ISR functions and vector table entries.
