# AEM Language Cheat Sheet
- **Angles**: u8 (0-255) for 0-360 degrees.
- **Math**: Supports i8-i64, u8-u64, f32, f64.
- **Casting**: Mandatory 'as' keyword.
- **Queues**: SPSC, Lock-free (overwrite_old | discard_new).
- **Trig**: Trig.sin_i(u8) for fast math, Trig.sin_f(f32) for precision.
