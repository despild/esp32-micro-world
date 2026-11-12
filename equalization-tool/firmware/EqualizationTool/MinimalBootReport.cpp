// SPDX-License-Identifier: GPL-2.0-or-later
// Arduino ESP32 3.1.1's main.cpp references both report functions even when
// shouldPrintChipDebugReport() is false. Satisfy those references here so the
// static linker need not pull chip-debug-report.cpp and its peripheral dump
// dependencies into our existing 1 MiB application partition.
// Only the optional verbose core boot report is omitted. USB STATUS, SELFTEST,
// HTTPTEST and application error reporting remain available.
// Revisit these entry points before changing the pinned Arduino core version.
void printBeforeSetupInfo() {}
void printAfterSetupInfo() {}
