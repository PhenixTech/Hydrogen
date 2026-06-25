# Hydrogen V1

> *A CR2032-powered keychain clock, minimalist, open hardware, and built to last.*


![Hardware](https://img.shields.io/badge/HARDWARE-open-blue?style=plastic) 
![MCU](https://img.shields.io/badge/MCU-CH32V006-green?style=plastic)
![Architecture](https://img.shields.io/badge/RISC--V-RV32EC-red?style=plastic)

<img src="images/PCB-3D-TOP.png" width="25%"> <img src="images/IRL-ON.jpg" width="25%"> <img src="images/IRL-BK.jpg" width="25%">

---


## Overview

![BANNER](images/BANNER.png)

**Hydrogen V1** is a coin-cell (CR2032) powered keychain clock designed around aggressive power optimization and a minimal BOM. It wakes on button press, displays the time on an SSD1306 OLED for 5 seconds, then returns to deep sleep, targeting a **~6-month battery life** on a single CR2032.

This is the first device in the **Hydrogen** line, part of the broader [PhenixTech](https://phenixtech.fr) hardware project ecosystem.

---

## Hardware

### Block Diagram

```
CR2032
  │
  ├──[BAT54 Schottky]──► VBAT
  │        │
  │        └── [22µF holdover cap]──► PCF8563T VBAT 
  │                                  
  ├──► CH32V006 (TSSOP20)
  │       │
  │       ├── I²C ──► PCF8563T RTC
  │       ├── I²C ──► SSD1306 OLED (daughter board)
  │       ├── GPIO ──► LED (3kΩ, ~500uA, low-battery indicator)
  │       └── SWIO/RST pads (debug header)
  │
  └──► 3215 32.768kHz crystal (RTC clock source)
```

### Bill of Materials (core)

| Component | Part | Package | Unit Cost |
|---|---|---|---|
| MCU | CH32V006F8P6 | TSSOP20 | €0.165 |
| RTC | PCF8563 | SOP8 | €0.155 |
| Crystal | 32.768kHz SMD | 3215 | €0.10 |
| Schottky | BAT54 | SOD-23 | €0.012 |
| LED | Red, 0603 | 0603 | €0.007 |
| Nav | 3-way thumbwheel | SMD | €0.057 |
| **Total (timing/control)** | | | **~€0.50** |

*Display module (SSD1306 0.96" I²C) sourced separately. (about 0.50€ each)*

### Key Design Decisions

- **Two-PCB approach** — main board + SSD1306 module daughter board with a 4 pin headers. V2 will integrate a bare OLED panel on a single PCB.
- **BAT54 Schottky** — keeps the RTC domain isolated for the holdover cap.
- **22µF holdover cap** — keeps the RTC alive for ~20 seconds during a battery swap, preserving time (also allow a clean shutdown).
- **Debug interface** — three  pads (SWIO, GND, RST) with a 2.54mm pitch for WCH-LinkE programming.

<img src="images/Schematic_Hydrogen-1_2026-04-07.png" width="50%">

---

## Firmware

> Developed with the WCH SDK / MounRiver Studio II

### Behavior

```
[Power on / button press]
        │
        ▼
  Wake from deep sleep
        │
        ▼
  Read time via I²C (PCF8563T)
        │
        ▼
  Display on SSD1306 (5 seconds)
        │
        │
        │
        ▼
  Return to deep sleep
```

### Power Budget

| State | Current | Duration |
|---|---|---|
| Deep sleep (MCU + RTC) | 60uA | ~99% of time |
| Active (OLED on, MCU running) | 8mA | 5s per wake |
| LED flash (low-battery) | ~500uA | brief pulse |

*Estimated battery life: **~6 months** on a standard CR2032 (225mAh).*
---

## PCB

Designed in **EasyEDA**, fabricated by **JLCPCB**, components sourced from **Aliexpress**.

- 2-layer PCB, 1.6mm thickness
- SMD components on top side; CR2032 holder on bottom
- Compact form factor for keychain use
- Two-board stack: main board + OLED module daughter board

<img src="images/PCB-TOP.png" width="25%"> <img src="images/PCB-BOTTOM.png" width="25%"> 

---

## Enclosure (planned)

A two or three-part enclosure is in design:

- **Back shell** — resin cast
- **Front plate** — plastic with OLED window and LED cutout
- **Optional** — magnetic swappable faceplate system

---

## Roadmap

| Version | Status | Key Changes |
|---|---|---|
| **V1** | ✅ | Two-board stack, SSD1306 module |
| **V2** | 🔲 Planning | Single PCB, bare OLED via FPC, MOSFET power gating, reverse polarity protection, optimized layout |

---

## Project Naming

Hydrogen is part of my own hardware ecosystem, where projects are named after elements and subatomic particles:

- **Boron** — main handheld line (RP2350, ST7789 display)

---
