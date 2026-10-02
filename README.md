# RawTime

A clean, minimalist digital watchface for the [UNA Watch](https://unawatch.com).

```
+-----------------------------------+
|                                   |
|                                   |
|             10 : 42               |   <- Centered Time (12h/24h)
|                                   |
|             FRIDAY                |   <- Weekday
|             OCT 02                |   <- Date (Month & Day)
|                                   |
|             [====|]               |   <- Battery Indicator
|                                   |
+-----------------------------------+
```

## Features

- **Centered Digital Clock**: Bold, monospaced time readout (`IBMPlexMono_Medium_36`) with persistent colon separator. Supports both 12-hour (with `am`/`pm` meridiem indicator) and 24-hour formats.
- **Date Underneath**: Two-line clean date display with full weekday name and month/day number, automatically adopting the user's system date order (`Month Day` or `Day Month`).
- **Battery Indicator**: Bottom-center vector battery icon with 4 level segments matching UNA OS glance standards:
  - `75% - 100%`: 4 segments (Teal)
  - `50% - 74%`: 3 segments (Teal)
  - `25% - 49%`: 2 segments (Teal)
  - `1% - 24%`: 1 segment (Red alert)
- **Power Efficient**: Follows UNA Watch power constraints ("Do not subscribe to what you do not draw") — subscribes exclusively to `BATTERY_LEVEL` sensor events and updates the clock once per minute.

## Architecture

Built using the UNA Watch SDK two-process model:

1. **Service (`RawTimeService.elf`)**: Background daemon on the RTOS kernel. Reads local time, monitors battery level, queries system settings, and sends IPC messages.
2. **GUI (`RawTimeGUI.elf`)**: Foreground TouchGFX process that handles layout, typography, and draws the user interface.

## Building

### Prerequisites

- [UNA Watch SDK](https://github.com/UNAWatch/una-sdk)
- ST ARM GCC Toolchain (Cortex-M33, e.g. from STM32CubeCLT)
- CMake 3.21+ and GNU Make
- Python 3 with requirements from `$UNA_SDK/Utilities/Scripts/app_packer/requirements.txt`

### Build Instructions

```bash
# 1. Set UNA SDK path
export UNA_SDK="/path/to/una-sdk"

# 2. Configure build
cmake -B build -S Software/Apps/RawTime-CMake -DCMAKE_TOOLCHAIN_FILE="$UNA_SDK/cmake/arm-none-eabi.cmake"

# 3. Build and package .uapp
cmake --build build
```

The compiled package will be generated at:
```text
Output/RawTime_0.0.1.uapp
```

## License

This project is licensed under the Apache 2.0 License - see the UNA Watch SDK documentation for details.
