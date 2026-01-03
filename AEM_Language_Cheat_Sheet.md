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
| `struct`      | Defines a data structure, can contain variables (`let`/`let mut`) and functions (`fn`). Can implement `interface`s.                | `struct Point { let x: i16; let y: i16; }`     |
| `queue`       | Declares a statically allocated, single-writer, single-reader buffer.                                                                | `queue Readings <i16, 16> [overwrite_old];`    |
| `fn`          | Defines a function. Supports hoisting.                                                                                               | `fn calculate(i16: val) : i16 { return val; }` |
| `let`/`let mut` | Declares an immutable (`let`) or mutable (`let mut`) variable or constant.                                                           | `let count: u8 = 0; let mut state: bool = false;` |
| `const`       | Declares a compile-time constant.                                                                                                    | `const MAX_VAL: u16 = 255;`                    |
| `module`      | Implicitly defined by file structure (`my_file.aem` creates `my_file::` namespace).                                                  | `use stdlib::Clock;`                           |
| `use`         | Imports modules or specific symbols into the current scope.                                                                          | `use my_module::MyStruct;`                     |
| `return`      | Returns a value from a function.                                                                                                     | `return true;`                                 |
| `if`/`else if`/`else` | Conditional branching. Mandatory braces `{}`.                                                                                | `if (x > 0) { ... } else { ... }`              |
| `loop`        | Infinite loop.                                                                                                                       | `loop { ... }`                                 |
| `for`         | Range-based loop.                                                                                                                    | `for i in 0..10 { ... }`                       |
| `while`       | Conditional loop.                                                                                                                    | `while (condition) { ... }`                    |
| `break`/`continue` | Control loop execution. Supports labeled breaks.                                                                                     | `loop 'outer { ... if (x) break 'outer; }`   |
| `match`       | Exhaustive pattern matching. Replaces C++ `switch`. No fall-through.                                                                | `match x { 0..10 => { ... }, _ => { ... } }`   |
| `as`          | Explicit type casting.                                                                                                               | `(my_i8 as u16) + (my_u8 as u16)`              |

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

## 8. Error Handling
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
## 9. Standard Library Modules

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

### `Trig` Module (integer-only trigonometry using u8 for angles)
Angles are `u8` (0-255 representing 0-360 degrees, "binary degrees"). Results are `i16` scaled by 10,000.
*   `Trig.sin(u8: angle) : i16`
*   `Trig.cos(u8: angle) : i16`
*   `Trig.tan(u8: angle) : i32`
*   `Trig.atan2(i16: y, i16: x) : u8`

### Standard Constants (Accessed via `const` or `module::Constant`)
*   `SYS_FREQ` - CPU Frequency in Hz.
*   `BOARD_NAME` - String name of the target board.
*   `TICK_RATE` - Scheduler tick resolution (e.g., 1ms).

## 10. Transpilation to C++ Considerations
*   **Static Polymorphism**: Interfaces map to C++ templates or concepts, avoiding runtime overhead.
*   **Queues**: Map to specialized, fixed-size C++ ring buffer classes optimized for SPSC.
*   **Tasks**: Map to C++ objects registered with a non-preemptive scheduler (e.g., Arkipenko's).
*   **Integer Math**: AEM bit-shifts map directly to C++ bit-shifts.
*   **Error Handling**: AEM `Result` pattern maps to `std::optional` (if available and no overhead) or custom error `struct`s.
*   **Interrupts**: `hardware` block definitions generate specific C++ ISR functions and vector table entries.
