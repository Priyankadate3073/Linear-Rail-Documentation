# Linear Actuator

A position-controlled linear actuator with homing, target positioning, and holding force, driven by an Arduino and L298 motor driver.

---

## Components

| Component | Description |
|-----------|-------------|
| Linear Actuator | NEMA 17 - 2000mm stroke |
| End Switch | Limit switch used for homing reference |
| Arduino Motor Shield | L298N Motor Driver |
| Arduino | Arduino Uno |


---

## Wiring

### Pin Connections

| Arduino Pin | Connects To | Function |
|-------------|-------------|----------|
| `A+` | Black Motor Wire | Motor Connections |
| `A-` | Green Motor Wire | Motor Connections |
| `B+` | Red Motor Wire | Motor Connections |
| `B-` | Blue Motor Wire | Motor Connections |
| `2` | End Switch pin NO | End Switch Connections |
| `GND` | End Switch pin COM | End Switch Connections |
| `Vin` | DC Power Supply L298N | Power Supply |
| `GND` | DC Power Supply ground | Power Supply |


### Power

Required Power - **12V, 2A DC power supply**.

---

## Software Setup

### Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) (version 2.3.8 recommended)
- `Stepper.h` library (built-in, no installation needed)

---

## How to Use

### Step-by-step guide

**1. Connect power**
 
Connect the provided power connectors to a **12V, 2A DC power supply**. Then connect the Arduino to your laptop using the USB cable.

**2. Upload the Code**
 
1. Download or clone this repository.
2. Open `motor_shield_holding.ino` in the Arduino IDE.
3. Select your board under **Tools → Board** (e.g. *Arduino Uno*).
4. Select the correct port under **Tools → Port**.
5. Click **Upload** (→ arrow button).

**3. Open the Serial Monitor**
 
In the Arduino IDE, open **Tools → Serial Monitor**. Set the baud rate to **115200** (bottom-right dropdown). You should see:
 
```
=== FPB30 Stage ===
Home first! Send H
```

**4. Home the actuator**
 
Type `H` and press Enter. The actuator will move toward the end switch, find it, and set that position as **0 mm**. You must home before any other movement command will work.
 
> Do not obstruct the actuator during homing. Keep hands and objects clear of its travel path.
 
**5. Move the actuator**
 
Once homed, you can control the actuator using the commands below. Type a command in the Serial Monitor and press Enter.
 
### Serial Commands
 
| Command | Example | Description |
|---------|---------|-------------|
| `H` | `H` | Home the actuator (must be done first) |
| `M<mm>` | `M150` | Move to an absolute position in mm (0–2000) |
| `L<mm>` | `L25` | Set the step distance for the `G` command (default: 10 mm) |
| `G` | `G` | Move by the set step distance in the current direction |
| `D<0\|1>` | `D1` | Set direction: `D1` = forward, `D0` = backward |
| `R<rpm>` | `R120` | Set motor speed in RPM (default: 60) |
| `K` | `K` | Toggle holding force on/off |
| `W<0–255>` | `W180` | Set holding force strength (default: 255 = full) |
| `C` | `C` | Start continuous movement in the current direction |
| `S` | `S` | Stop all movement and disable hold |
| `?` | `?` | Show current position, homing status, and hold status |

### Typical workflow
 
A holding force of `W140` is sufficient to prevent accidental movement in most cases.
 
1. Send `H` to home the actuator.
2. Send `W140` to set the holding force strength.
3. Send `K` to enable holding force.
4. Send `M100` to move to 100 mm.
5. Send `M0` to return to the home position.
6. Send `S` to stop and release the motor.

**6. Power off**
 
Send `S` to stop and release the motor. Disconnect the external power supply first, then USB. The actuator will lose holding force and may move slightly depending on the load.

## Configuration
 
These values can be adjusted in `motor_shield_holding.ino`:
 
| Parameter | Default | Description |
|-----------|---------|-------------|
| `STEPS_PER_REV` | `200` | Steps per motor revolution (Nema 17) |
| `MM_PER_REV` | `60.0` | Linear travel per revolution in mm (FPB30 belt drive) |
| `MAX_TRAVEL_MM` | `2000.0` | Maximum travel limit in mm |
| `targetRpm` | `60.0` | Default motor speed in RPM (adjustable via `R` command) |
| `targetDistance` | `10.0` | Default step distance in mm (adjustable via `L` command) |
| `homeRpm` | `60.0` | Motor speed during homing |
| `holdPwm` | `255` | Holding force strength, 0–255 (adjustable via `W` command) |
 
---

## Troubleshooting
 
| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Actuator doesn't move at all | No power to L298N, or wiring error | Check power supply and motor output connections |
| Actuator doesn't home | End switch not wired correctly or wrong pin | Verify switch wiring and pin assignment in code |
| Actuator moves the wrong direction | IN1/IN2 swapped | Swap the two motor wires at L298N output, or swap IN1/IN2 pin values in code |
| Actuator homes but doesn't reach target | Target value out of range | Adjust `TARGET_POSITION` to be within actuator stroke |
| Actuator vibrates or is jittery | Insufficient power supply current | Use a power supply rated above the actuator's stall current |
| Holding force is weak | `HOLDING_PWM` set too low | Increase the holding PWM value |
| L298N gets very hot | High holding PWM over long periods | Reduce `HOLDING_PWM` or add a heatsink; consider duty-cycling the hold |

---

## Safety
 
- **Pinch hazard:** Keep fingers and loose items away from the actuator's travel path during operation.
- **Do not manually force** the actuator while it is powered — this can damage the motor or driver.

---
