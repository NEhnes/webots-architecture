# WEBots ESP32 CAN Bus Communications
### _Visit [DRIVE FOLDER](https://drive.google.com/drive/u/0/folders/1rvsJLOoQhFb15k3eDJaLo-1WPcugmLJx) for build pics & videos_

A purpose-built CAN 2.0B communications layer for the WEBots humanoid robot. Designed as the foundation for distributed motor control and micro-ROS integration via custom serialization.

**Status**: Sender/receiver nodes operational with acknowledgment-based reliability. Next phase: micro-ROS custom CAN transport layer.

---

## The Problem

- Multiple servo/BLDC motor controllers (ankle, knee, subsystems) need noise-resistant comms over single bus
- WiFi/serial fail in high-noise environment: PWM signals, motor driver spikes, switching power supplies corrupt single-ended signals
- CAN measures **difference between two wires** (not absolute voltage) → noise immunity by design

**Solution: CAN 2.0B** (ISO 11898-1)
- Deterministic, differentially encoded, acknowledgment-based delivery guarantees message delivery
- Proven in automotive/industrial safety-critical systems

**Challenge**: Inherit hardware (ESP32s + MCP2515 modules), previous comms lead unfamiliar with tech stack, had to debug issues solo.

---

## Why CAN 2.0B

- **Differential signaling**: Twisted pair rejects common-mode noise (noise on both wires = cancels out)
- **Deterministic**: No WiFi retries, no buffer overflows, bounded latency
- **Scalable**: Multi-node single bus, just assign new CAN IDs, reduce electrical noise
- **Acknowledgment**: Hardware-native, guarantees critical commands reach destination

---

## Implementation

**Structure**: `ESP_CAN_SEND/` + `ESP_CAN_RECV/` on PlatformIO, autowp-mcp2515 lib, ESP32-DevKit.

### Sender

- Read joystick (12-bit ADC, VRX + VRY) → pack into 8-byte CAN frame (ID 0x036)
- Send, wait 500ms for ACK (ID 0x037), retry up to 3 times
- Update rate: 25 Hz (40ms loop, no need faster on local bus)

```cpp
canMsg.can_id = 0x036;
memcpy(&canMsg.data[0], &VRX, sizeof(VRX));  // Bytes 0–3
memcpy(&canMsg.data[4], &VRY, sizeof(VRY));  // Bytes 4–7
```

### Receiver

- Listen for ID 0x036, validate, send immediate ACK (0x037)
- Decode joystick → direction: compare X/Y magnitude, 500-unit deadzone, return UP/DOWN/LEFT/RIGHT/NEUTRAL
- Display arrow bitmap on SSD1306 OLED (visual confirmation)

```cpp
Direction getInput(int VRX, int VRY) {
  int x = VRX - 2048, y = -(VRY - 2048);
  if (abs(x) < 500 && abs(y) < 500) return Direction::NEUTRAL;
  return (abs(x) > abs(y)) ? (x > 0 ? RIGHT : LEFT) : (y > 0 ? UP : DOWN);
}
```

### Design Rationale

- **ACK**: Motor control demands reliability. Fire-and-forget hides failures. ACK surfaces problems immediately.
- **Hardware forward-compatibility**: Electrical team's custom PCB uses same transceiver (SN65HVD235D). Firmware works unchanged.
- **Deadzone**: 500 units (~12% range) filters noise without lag. Will tune per-subsystem once motors integrate.

---

## Building & Running

**Hardware**: ESP32-DevKit, MCP2515 CAN module, SSD1306 OLED (receiver only).

**Wiring** (both nodes):
- ESP32 → MCP2515: MOSI (23), MISO (19), SCK (18), CS (5), GND, 3.3V

**Receiver → OLED**:
- SDA (21), SCL (22), GND, 3.3V

**Build**:
```bash
cd ESP_CAN_SEND && platformio run --target upload
cd ESP_CAN_RECV && platformio run --target upload
```

**Verify**: Serial monitor (115200 baud). Sender: `ACK received`. Receiver: joystick values decoded.

---

## Roadmap: Toward micro-ROS

**Phase 2** (in progress): micro-ROS serial connection, ROS2 nodes on ESP32.

**Phase 3** (design): Custom CAN transport—fragment ROS2 messages (>64B) into CAN frames (8B), reassemble with per-frame ACK.

**Phase 4**: Test with electrical team's custom PCB on real servo/BLDC loads.

---

## Status

**Working**: ✅ Sender transmits reliably ✅ Receiver decodes ✅ ACK functional ✅ OLED confirms fidelity

**Limitations**: No error logging yet; deadzone tuned for joystick (not motors); MCP2515 @ 1 Mbps max (adequate).

---

## Solo Build

Inherited hardware with minimal guidance. Debugged WSL kernel recompilation (CAN utilities), studied ROS2 architecture in parallel, incremented from simple CAN examples to stateful ACK protocols. Framework is solid for next developer to extend without rework.

---

## References

- **CAN 2.0B Spec**: ISO 11898-1 (differential signaling, arbitration, ACK mechanism)
- **MCP2515 Library**: [autowp/mcp2515](https://github.com/autowp/mcp2515)
- **micro-ROS**: https://micro.ros.org/
- **PlatformIO**: https://platformio.org/

---

**Last Updated**: January 2026  
**Status**: Active development, micro-ROS integration in progress
