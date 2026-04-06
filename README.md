# STM32 Embedded Chess ♟️

A complete, standalone chess engine and graphical user interface running on a deeply constrained microcontroller. This project combines a custom hardware-optimized Alpha-Beta search algorithm with a modern touchscreen UI, utilizing a Real-Time Operating System to handle concurrent engine calculation and graphical rendering.

# Demo video

# Function

This project splits the workload using FreeRTOS. One task manages the LVGL display and touchscreen inputs, while a background task runs the heavy chess tree calculations without blocking the user interface. 

##  Engine Search Features:

- Alpha-Beta Pruning (Negamax)
- Quiescence Search
- Move Ordering 
    - PV (Principal Variation) Ordering
    - MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    - Killer Heuristic
- not implemented yet:
  - cacheing
  - book move
  - hashtable lookup
  - polykey

## GUI & User Experience:
- Interactive touchscreen play against the microcontroller.
- Move validation and piece rendering.
- "Take back" move functionality

# Hardware 
- STM32F429 Discovery (STM32F429I-DISCO)
- ARM Cortex-M4 Core
- Onboard 2.4" QVGA TFT LCD with Touchscreen

# Credit
- Chess Engine: **VICE**, (https://github.com/bluefeversoft/vice)
- GUI Graphics Library: **LVGL**
- GUI Layout Design: **EEZ Studio**
- Concurrency / RTOS: **CMSIS FreeRTOS**