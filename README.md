# ServoHeadController

Controls 2 servos in the Diorama movement head.

## Hardware

| Item | Detail |
|------|--------|
| Target MCU | Arduino Pro Mini (ATmega328P, 5 V / 16 MHz) |
| Servo library | Standard Arduino `Servo` library |

### Pin assignments

| Pin | Direction | Function |
|-----|-----------|----------|
| D9  | Output | Servo 1 signal (PWM) |
| D10 | Output | Servo 2 signal (PWM) |
| D2  | Input  | On/Off demand from external hardware (active low) |
| D4  | Output | Servo power enable (active high) |

### Behaviour

* Both servos initialise to their **Off** positions at power-up.  
* Servo power is enabled only **after** initialisation is complete.  
* When the demand input goes low the servos move smoothly to their **On**
  positions over 5 seconds.  
* When the demand input goes high the servos return smoothly to their
  **Off** positions over 5 seconds.

Servo positions and GPIO pins are defined as `#define` constants at the top
of `src/main.cpp` and can be adjusted without touching any other code.

---

## Prerequisites

1. **Git** – <https://git-scm.com/downloads>  
2. **Visual Studio Code** – <https://code.visualstudio.com/>  
3. **PlatformIO IDE extension for VS Code** –
   search for *PlatformIO IDE* in the VS Code Extensions panel, or visit
   <https://platformio.org/install/ide?install=vscode>

---

## Getting a local copy

```bash
git clone https://github.com/swatso/ServoHeadController.git
cd ServoHeadController
```

---

## Building and uploading

### With VS Code + PlatformIO IDE

1. Open VS Code and choose **File → Open Folder…**, then select the
   `ServoHeadController` folder.
2. PlatformIO will automatically detect `platformio.ini` and install the
   required toolchain and libraries.
3. Connect your Arduino Pro Mini via a USB-to-serial adapter.
4. Click the **Upload** button (→) in the PlatformIO toolbar, or press
   `Ctrl+Alt+U`.

### With the PlatformIO CLI

```bash
# Install dependencies and build
pio run

# Build and upload to the connected board
pio run --target upload

# Open the serial monitor (optional)
pio device monitor
```

> **Note:** If you are using the **3.3 V / 8 MHz** variant of the Pro Mini,
> change the environment in `platformio.ini` to `pro8MHzatmega328` before
> building.
