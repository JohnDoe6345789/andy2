# libIOTCAPIsT Library Overview

## Source (`libIOTCAPIsT.c`)
`libIOTCAPIsT.c` provides a growing clean-room re-implementation of a subset of
the original shared library.  Only functionality required for unit tests is
modelled.  Currently implemented features include:

- Stack-protected wrapper around `IOTC_Session_Read`
- Simplified SSL shutdown handling via `IOTC_sCHL_shutdown`
- Endianness helpers (`IOTC_Data_{ntoh,hton}` and header conversions)
- Lightweight in-memory session management with channel toggling
- Retrieval of library version information
- Allocation of session identifiers and dynamic sizing of the session table

The goal of this file is to allow testing without the proprietary runtime.

## Shared Library (`libIOTCAPIsT.so`)
`libIOTCAPIsT.so` is the compiled implementation used by the Android app. It is
a 64-bit AArch64 ELF shared object and exports a large set of functions related
to TUTK's P2P IoT connection framework, including NAT traversal, UDP/TCP
connection tasks, and session management. Example exported symbols include:

- `AddUDPRelayConnectTask`
- `IOTC_Client_Connect_By_Nebula`
- `IOTC_Connect_ByUID`
- `IOTC_Connect_UDP`

These routines allow the application to establish and manage device connections over the TUTK platform.

## Usage
The Android application loads `libIOTCAPIsT.so` at runtime to perform networking operations. The stub C file can be used as a reference for available entry points or for building wrappers during analysis or reverse engineering.
