---
title: "Development"
description: "Guides for building, extending, and contributing to PocketMage."
---

# Development

This section covers everything you need to build PocketMage from source, create OTA apps, and contribute to the project.

## Build Environments

How to set up PlatformIO and compile the firmware on Linux, macOS, and Windows.

- [Build Environments & PlatformIO Setup](build-environments.md)

## OTA App Development

The PocketMage supports sideloading apps into OTA partitions. An app template and video guide are available:

- [APP_TEMPLATE.cpp](https://github.com/ashtf8/PocketMage_PDA/blob/main/Code/PocketMage_V3/src/APP_TEMPLATE.cpp) - reference template for creating OTA apps
- [Developing For the PocketMage](https://www.youtube.com/watch?v=3Ytc-3-BbMM) - video walkthrough

## Source Code Overview

The main firmware lives in `Code/PocketMage_V3/`. Key directories:

- `src/` - application source (TXT, FileWiz, Calendar, etc.) and the PocketMage OS kernel
- `lib/` - third-party libraries (Adafruit GFX, U8g2, TCA8418, Mesh-NOW)
