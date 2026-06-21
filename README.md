---
title: "WEBots ESP32 Robot Communications Architecture"
date: "2026-06-20"
team_size: 1
scope: "ESP32-based CAN bus and micro-ROS communication layers for the WEBots humanoid robot platform"
skills: ["Embedded C++", "CAN 2.0B", "micro-ROS", "ROS 2", "PlatformIO"]
---

## Key Results / Highlights

- Built a **CAN 2.0B sender/receiver pair** for joystick telemetry using ESP32, MCP2515, and a **500 kbps** bus.
- Added **ACK-based reliability** with retry handling, giving command delivery visibility instead of fire-and-forget transmission.
- Integrated **micro-ROS on ESP32** with stable serial transport to a full **ROS 2 Jazzy** agent.
- Demonstrated bidirectional ROS behavior: ESP32 publishes joystick and string topics, and responds to LED control commands.
- Designed the communication stack to scale from prototype modules to the electrical team's future custom PCB.

## Table of Contents

- [Project Overview](#project-overview)
- [Skills Demonstrated](#skills-demonstrated)
- [Technical Summary](#technical-summary)
  - [CAN Bus Layer](#can-bus-layer)
  - [micro-ROS Layer](#micro-ros-layer)
  - [Repository Structure](#repository-structure)
- [Design Challenges](#design-challenges)
- [Build and Run](#build-and-run)
- [Status and Roadmap](#status-and-roadmap)
- [References](#references)

## Project Overview

| Field | Details |
|---|---|
| Project | WEBots ESP32 Robot Communications Architecture |
| Platform | ESP32-DevKit / ESP32DOIT DevKit |
| Tools | PlatformIO, Arduino framework, ROS 2 Jazzy, micro-ROS |
| Communication | CAN 2.0B over MCP2515, serial micro-ROS transport |
| Scope | Prototype distributed communications for humanoid robot control and sensing |

This repository combines two communication experiments for the WEBots humanoid robot: a **CAN 2.0B bus layer** for noise-resistant local control data and a **micro-ROS layer** for integration with ROS 2. The CAN system focuses on deterministic joystick-style telemetry between ESP32 nodes, while micro-ROS validates that the ESP32 can participate in a ROS 2 graph through topics and services.

The work is positioned as a foundation for future motor-controller integration, where multiple ankle, knee, and subsystem controllers need reliable communication under motor-drive electrical noise.

## Skills Demonstrated

### Embedded C++

- Implemented ESP32 firmware in C++ using **Arduino framework**, SPI initialization, GPIO configuration, timers, and serial debugging.
- Packed joystick ADC readings into fixed 8-byte CAN frames using explicit memory layout and deterministic loop timing.

### CAN Bus and Hardware Interfaces

- Configured **MCP2515 CAN controllers** at **500 kbps** with SPI chip-select on GPIO 5.
- Built a sender/receiver protocol using CAN ID `0x036` for joystick telemetry and `0x037` for acknowledgments.

### ROS 2 and micro-ROS

- Initialized a micro-ROS node, publishers, subscriber, timers, executor, and service client on ESP32.
- Connected ESP32 topics to a full **ROS 2 Jazzy** install over serial transport.

### Testing and Debugging

- Verified CAN behavior through serial logs, OLED visual feedback, and ACK/retry checks.
- Debugged embedded stack integration issues while moving from standalone CAN examples to stateful communication.

## Technical Summary

The project is split into two complementary firmware layers. `can-bus/` implements a low-level, noise-resistant communications path for local robot control data. `microros/` implements a higher-level ROS 2 integration path using micro-ROS serial transport.

### CAN Bus Layer

- **Sender node**: reads joystick X/Y ADC values, serializes them into an 8-byte CAN frame, transmits on ID `0x036`, waits up to **500 ms** for ACK ID `0x037`, and retries up to **3 times**.
- **Receiver node**: validates incoming CAN ID `0x036`, decodes X/Y values, sends an immediate ACK, applies a **500-unit deadzone**, and displays direction on an SSD1306 OLED.
- **Timing**: sender loop runs at roughly **25 Hz** with a 40 ms delay, prioritizing stable local telemetry over maximum update rate.

```cpp
canMsg.can_id  = 0x036;
canMsg.can_dlc = 8;
memcpy(&canMsg.data[0], &VRX, sizeof(VRX));
memcpy(&canMsg.data[4], &VRY, sizeof(VRY));
```

### micro-ROS Layer

- **Publishers**: `/joystick_x_pub` publishes joystick X-axis values every **50 ms**; `/goose_calls` publishes `"HONK"` every **400 ms**.
- **Subscriber**: `/led_control` accepts `"ON"` and `"OFF"` strings and drives an LED accordingly.
- **Service exploration**: defines a manual `Average4Request` / `Average4Response` interface for a custom `average4` service, with the service-client path prepared for ROSIDL-generated types.

```cpp
rclc_publisher_init_default(
  &esp32_pub,
  &node,
  ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
  "joystick_x_pub");
```

### Repository Structure

```text
webots-architecture/
├── can-bus/
│   ├── ESP_CAN_SEND/
│   │   └── src/main.cpp        # joystick sender, CAN TX, ACK retry logic
│   └── ESP_CAN_RECV/
│       └── src/main.cpp        # CAN RX, ACK response, OLED direction display
└── microros/
    ├── src/Main.cpp            # micro-ROS node, publishers, subscriber, service client
    └── include/average4_request.hpp
```

## Design Challenges

- **Electrical noise in robot control buses** → *Single-ended serial or WiFi links are vulnerable near PWM signals, motor drivers, and switching power supplies* → *CAN differential signaling adds wiring complexity but directly targets noise immunity* → Chose **CAN 2.0B over MCP2515** because it is deterministic, widely supported, and compatible with future custom PCB transceivers.
- **Reliable local command delivery** → *A fire-and-forget CAN frame can hide dropped commands during integration* → *Adding application-level ACK costs bus time but exposes delivery failures immediately* → Implemented ACK ID `0x037` with a **500 ms timeout** and **3 retries**.
- **Joystick drift and deadzone tuning** → *Raw ADC values fluctuate around center and can create false direction changes* → *A larger deadzone improves stability but reduces sensitivity* → Used a **500-unit deadzone** around center as a practical prototype threshold.
- **Moving from CAN prototypes to ROS 2 integration** → *CAN is excellent for local control, but higher-level robot behavior needs ROS graph integration* → *micro-ROS serial transport is simpler than custom CAN transport but depends on a host agent* → Built the micro-ROS node first with topics and LED control, leaving custom CAN transport as the next scaling step.
- **Solo debugging across unfamiliar stacks** → *The project combined embedded firmware, CAN controllers, ROS 2, and micro-ROS tooling* → *A broad stack increases learning value but slows integration* → Incremented from simple CAN examples to ACK behavior, then added micro-ROS topics and service structures step by step.

## Build and Run

### CAN Bus

**Hardware**

- ESP32-DevKit boards
- MCP2515 CAN modules on both sender and receiver
- SSD1306 OLED on receiver
- Joystick connected to sender ADC pins
- 3.3V logic wiring

**Wiring**

| Signal | ESP32 Pin | MCP2515 |
|---|---:|---|
| SCK | GPIO 18 | SCK |
| MISO | GPIO 19 | SO |
| MOSI | GPIO 23 | SI |
| CS | GPIO 5 | CS |
| GND | GND | GND |
| 3.3V | 3.3V | VCC |

**Receiver OLED**

| Signal | ESP32 Pin | OLED |
|---|---:|---|
| SDA | GPIO 21 | SDA |
| SCL | GPIO 22 | SCL |
| GND | GND | GND |
| 3.3V | 3.3V | VCC |

**Build**

```bash
cd can-bus/ESP_CAN_SEND && platformio run --target upload
cd can-bus/ESP_CAN_RECV && platformio run --target upload
```

**Verify**

- Sender serial monitor at **115200 baud** should show `ACK received`.
- Receiver serial monitor should show decoded joystick values and direction decisions.
- Receiver OLED should display `UP`, `DOWN`, `LEFT`, `RIGHT`, or `NEUTRAL`.

### micro-ROS

**Hardware**

- ESP32DOIT DevKit
- Serial connection to a host running the ROS 2 Jazzy micro-ROS agent
- Optional LED on GPIO 18 for `/led_control`

**Build**

```bash
cd microros && platformio run --target upload
```

**Run host agent**

```bash
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0
```

**Verify ROS topics**

```bash
ros2 topic list
ros2 topic echo /joystick_x_pub
ros2 topic echo /goose_calls
ros2 topic pub /led_control std_msgs/msg/String "{data: 'ON'}"
```

## Status and Roadmap

### Working

- CAN sender transmits joystick telemetry.
- CAN receiver decodes telemetry and sends ACKs.
- Receiver OLED confirms decoded direction.
- micro-ROS node initializes with a ROS 2 Jazzy agent.
- ESP32 publishes `/joystick_x_pub` and `/goose_calls`.
- ESP32 subscribes to `/led_control`.

### Limitations

- Error logging is not yet centralized.
- Joystick deadzone is tuned for the prototype joystick, not motor-controller feedback.
- The micro-ROS service client is prepared but not fully exercised in the active loop.
- No custom ROS 2-over-CAN fragmentation/reassembly layer is implemented yet.

### Roadmap

- Add structured error counters for CAN send failures, ACK timeouts, and malformed frames.
- Tune deadzone and scaling per subsystem before motor integration.
- Complete the custom `average4` service client path using generated ROSIDL types.
- Design a CAN transport layer that fragments ROS 2 messages larger than 8 bytes and reassembles them with per-frame ACKs.
- Validate firmware against the electrical team's custom PCB and real servo/BLDC loads.

## References

- ISO 11898-1 / CAN 2.0B concepts: differential signaling, arbitration, and ACK mechanism.
- MCP2515 library: `autowp/mcp2515`
- micro-ROS: https://micro.ros.org/
- PlatformIO: https://platformio.org/
- ROS 2: https://docs.ros.org/
