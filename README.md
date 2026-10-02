# RawTime - Custom Watchface Project for UNA Watch

A standalone starter project for creating a custom digital watchface for the UNA Watch platform.

This project is decoupled from the SDK examples tree and ready to be customized and built independently using the UNA SDK toolchain.

---

## Overview

On the UNA Watch platform, a **Clockface** is a specialized `.uapp` application (`APP_TYPE "Clockface"`) that the kernel renders as the primary home screen whenever no other app is active.

### Key Characteristics of a Clockface

- **Kernel owns physical buttons**: While a clockface is displayed, button presses navigate the OS (e.g. opening the top menu or glance list). Clockfaces receive no button events and have exactly one screen.
- **Power is the primary design constraint**: Watchfaces run continuously on screen for hours. The display and CPU only update when values actually change (typically once per minute for the clock, or on incoming sensor events).
- **Two-process architecture**:
  1. **Service (`RawTimeService.elf`)**: Background service running on the RTOS kernel. Reads system time, subscribes to sensors, fetches system settings, and sends updates over IPC.
  2. **GUI (`RawTimeGUI.elf`)**: Foreground process using TouchGFX. Receives updates from the service and draws the UI.
- **Suspension model**: When the user opens a menu or launches an app, the clockface is **suspended** (not terminated). When returning to the watchface, it resumes and requests fresh state via a `Refresh` message.

---

## Project Structure

```
RawTime/
├── README.md                   # This guide
├── Resources/
│   ├── icon_30x30.png          # Small launcher icon (required by packer)
│   └── icon_60x60.png          # Normal launcher icon (required by packer)
├── Output/                     # Default output directory for compiled .uapp
└── Software/
    ├── Apps/
    │   ├── RawTime-CMake/
    │   │   └── CMakeLists.txt  # Main build configuration (service + GUI + packaging)
    │   └── TouchGFX-GUI/       # TouchGFX UI project
    │       ├── RawTimeGUI.touchgfx
    │       ├── application.config
    │       ├── target.config
    │       ├── touchgfx.cmake  # CMake include for TouchGFX sources
    │       ├── assets/         # Fonts, images, and localized text definitions
    │       ├── gui/            # Hand-written UI code (MVP pattern)
    │       │   ├── include/gui/common/RawTimeLabels.hpp
    │       │   ├── include/gui/main_screen/
    │       │   ├── src/main_screen/
    │       │   └── src/model/
    │       └── generated/      # Pre-generated TouchGFX code
    └── Libs/                   # Service logic and shared IPC headers
        ├── libs.cmake          # Service build script
        ├── Header/
        │   ├── Commands.hpp    # CustomMessage IPC structures
        │   └── Service.hpp     # Service class declaration
        └── Sources/
            └── Service.cpp     # Service loop, clock timing, and sensor events
```

---

## Quickstart: Building the Project

### Prerequisites

1. **ST ARM GCC Toolchain** (from STM32CubeCLT or STM32CubeIDE) in your `PATH`.
2. **CMake 3.21+** and **GNU Make**.
3. **Python 3** with packer dependencies installed:
   ```bash
   python3 -m pip install -r "$UNA_SDK/Utilities/Scripts/app_packer/requirements.txt"
   ```
4. **`UNA_SDK`** environment variable exported:
   ```bash
   export UNA_SDK="/home/scott/repos/una-sdk"
   ```

### Build Commands

From inside the `RawTime` directory:

```bash
# 1. Create build directory and configure with CMake
cmake -G "Unix Makefiles" \
      -S Software/Apps/RawTime-CMake \
      -B build

# 2. Compile the service, GUI, and package into .uapp
cmake --build build
```

Upon a successful build, the packaged application binary will be produced:
```text
build/RawTime_0.0.0-dev.uapp
```

### Clean Rebuild

```bash
rm -rf build
cmake -G "Unix Makefiles" \
      -S Software/Apps/RawTime-CMake \
      -B build
cmake --build build
```

---

## Architecture & Data Flow

```text
  [Sensor Layer] ──EVENT_SENSOR_LAYER_DATA──► Service ◄── std::time(), once a minute
                                                 │
                                                 │ CustomMessage::Time
                                                 │ CustomMessage::Steps
                                                 │ CustomMessage::ClockFormat
                                                 ▼
  [Kernel] ──EVENT_GUI_TICK──► GUI (TouchGFX)
                                   │
                                   ├── Model::tick() ──► checks incoming IPC
                                   └── MainPresenter ──► updates MainView
```

### 1. Clock Timing Without Drift (`Service.cpp`)

Watchfaces update the clock only once per minute:

```cpp
while (true) {
    std::tm local {};
    readLocalTime(local);       // time() + localtime_r()
    publishTime(local);         // publishes only if minute changed

    uint32_t wait = msToNextMinute(local);  // wait until the start of the next minute

    SDK::MessageBase *msg;
    if (!mKernel.comm.getMessage(msg, wait)) {
        continue;
    }
    // Process incoming events...
}
```

- Publishing **before** the wait prevents boundary races.
- Sizing each wait from a fresh time reading avoids cumulative clock drift.

### 2. Startup Grace Period (`Service.cpp`)

The service process is started slightly before the GUI process. To avoid orphaned background services if a GUI fails to launch:

```cpp
if (!guiStarted) {
    const uint32_t elapsed = mKernel.sys.getTimeMs() - startTime;
    if (elapsed >= kStartupGraceMs) {
        LOG_INFO("GUI never started, exiting service\n");
        disconnect();
        return;
    }
}
```

### 3. Presentation Settings (12h/24h & Date Formats)

The user configures time format (12h vs 24h) and date format (Day-first vs Month-first) in the watch's **Settings -> Clock**. Because settings changes do not broadcast push events, the service requests them:
- At GUI startup (`COMMAND_APP_NOTIF_GUI_RUN`).
- On app resume via `CustomMessage::REFRESH`.
- Via a periodic background poll every 60 seconds (`kSettingsPollMs`).

### 4. Suspension & Resume (`Model.cpp`)

When the user enters a menu, the clockface GUI is suspended and stops receiving ticks:
```cpp
void Model::onResume()
{
    mResumed = true; // flag only: do not touch widgets here!
}

void Model::tick()
{
    if (mResumed) {
        mResumed = false;
        adopt(now());                                    // read fresh local time
        SDK::send_msg<CustomMessage::Refresh>(mKernel);  // ask service for fresh data
        application().invalidate();                      // repaint full framebuffer
    }
}
```

---

## Customizing Your Watchface

### 1. Changing the App ID and Metadata

In `Software/Apps/RawTime-CMake/CMakeLists.txt`:

```cmake
set(APP_NAME "RawTime")              # Internal target name
set(APP_USER_NAME "RawTime")         # Display name in Settings / Launcher (max 16 chars)
set(APP_TYPE "Clockface")            # Identifies app as a watchface
set(DEV_ID "UNA")
set(APP_ID "E59BC72570B21127")       # 16-character uppercase hex string
```

Generate a new unique 16-character hex ID for your app if needed:
```bash
python3 -c 'import hashlib; print(hashlib.md5(b"RawTime").hexdigest().upper()[:16])'
```

> [!IMPORTANT]
> If you test using the desktop simulator, update the matching `-DAPP_ID=` flag in `Software/Apps/TouchGFX-GUI/una/Makefile`.

### 2. Modifying the Display / Layout

- UI widgets and layout are defined in `Software/Apps/TouchGFX-GUI/gui/src/main_screen/MainView.cpp`.
- Positions and alignments for the clock and date are adjusted in `layoutClock()` and `layoutDate()`.
- Text strings, formatting functions, and translations are located in `Software/Apps/TouchGFX-GUI/gui/include/gui/common/RawTimeLabels.hpp`.

### 3. Adding Sensors (e.g. Battery Level)

To subscribe to battery level:

1. Add sensor connection in `Service.hpp`:
   ```cpp
   SDK::Sensor::Connection mBatterySensor;
   ```
2. In `Service::Service()`:
   ```cpp
   mBatterySensor(SDK::Sensor::Type::BATTERY_LEVEL);
   ```
3. Connect in `Service::connect()`:
   ```cpp
   mBatterySensor.connect(mKernel);
   ```
4. Parse in `Service::handleSensorData()`:
   ```cpp
   SDK::Sensor::DataParserBatteryLevel parser(batch);
   uint8_t charge = parser.getCharge();
   ```

> [!WARNING]
> **Do not subscribe to sensors you do not display.**
> In particular, subscribing to `HEART_RATE` keeps the optical PPG sensor powered, causing significant battery drain. Only subscribe to sensors actively drawn on screen.

### 4. IPC Message Size Rule

Kernel message pools have a strict upper limit of **256 bytes**. Keep all message structures in `Commands.hpp` small and verify with:
```cpp
static_assert(sizeof(CustomMessage::Time) <= 256, "Must fit kernel message pool");
```

---

## Deploying to UNA Watch

### Method 1: BLE Transfer via UNA Mobile App
1. Open the UNA Companion App on your mobile device.
2. Go to **Developer Settings** / **App Install**.
3. Select and transfer your built `.uapp` file.

### Method 2: USB Mass Storage
1. Connect the UNA Watch via USB.
2. Copy the `.uapp` into the watch's `Apps/` storage folder.
3. Unplug USB and select your watchface in **Settings -> Watch Face**.
