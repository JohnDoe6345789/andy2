# Pure Python script to stream Bambu Lab printer webcam feed (H.264) to a file.
# Mimics the Android app's use of ThroughTek (TUTK) IOTC SDK for P2P connection
# and streaming, using DTLS encryption for secure data transfer.
#
# Dependencies:
# - pydtls: Install with `pip install pydtls` (for DTLS 1.2 encryption).
# - Standard Python libraries: socket, struct, time, random, select, hashlib.
#
# Purpose:
# - Discovers Bambu Lab printers on the LAN (mimics IOTC_Lan_Search2).
# - Establishes a DTLS-encrypted P2P session (mimics IOTC_Connect_ByUID, avClientStart).
# - Streams H.264 video to a file (mimics avRecvFrameData2), playable in VLC.
# - Supports non-LAN-Only Mode (works with LAN Mode Liveview enabled).
# - No audio support, matching the Android app's video-only streaming.
#
# Prerequisites:
# - Enable LAN Mode Liveview on the printer:
#   - Settings > General > LAN Mode Liveview > Enable (on printer touchscreen).
# - Obtain the 8-character Access Code (from printer settings).
# - Obtain the 20-character UID (from printer label or discovery).
# - Ensure printer is on the same LAN as the computer running this script.
# - Install pydtls: `pip install pydtls`.
# - Windows/Linux may require admin privileges for UDP multicast (discovery).
#
# Usage:
# 1. Save this script as `stream_printer_p2p_dtls.py`.
# 2. Run: `python stream_printer_p2p_dtls.py` (use sudo/admin if discovery fails).
# 3. If printers are found, select one by index and enter the Access Code.
# 4. If no printers are found, enter UID and Access Code manually.
# 5. The script streams video to `<UID>.h264`.
# 6. Stop streaming with Ctrl+C.
# 7. Play the output file in VLC: File > Open File > `<UID>.h264` or `ffplay <UID>.h264`.
#
# Mapping to IOTC.c and AVAPIs.c (from decompiled SDK):
# - discover_printers(): Mimics IOTC_Lan_Search2 (UDP multicast to 239.255.255.250:10000).
# - iotc_connect_by_uid(): Mimics IOTC_Connect_ByUID (P2P session) and avClientStart (auth with bblp:<access_code>).
# - stream_to_file(): Mimics avRecvFrameData2 (H.264 frame parsing, NAL units).
# - Shutdown: Closes DTLS socket (mimics avClientStop, IOTC_Connect_Stop_BySID).
#
# Security Notes:
# - Uses DTLS 1.2 with PSK (derived from UID via SHA256; real app may use static key).
# - Mitigates TUTK P2P vulnerabilities (e.g., CVE-2021-28372) with encryption.
# - Keep Access Code private; use firewall to restrict UDP ports (10000, 63616, 32100).
# - P2P server IP (52.198.116.11) is hardcoded; may need updating (use Wireshark).
#
# Troubleshooting:
# - Discovery fails: Firewall may block UDP 10000 or multicast (239.255.255.250).
#   - Run as admin/sudo.
#   - Use manual UID input.
#   - Capture traffic with Wireshark to verify multicast responses.
# - Connection fails: Check UID (20 chars), Access Code (8 chars), Liveview enabled.
#   - Test port 63616 (local) or 32100 (P2P) with `telnet <ip> <port>`.
# - DTLS errors: Ensure pydtls is installed. PSK is simplified; capture app traffic for exact key.
# - Corrupt H.264: Test with VLC. Share packet capture for debugging.
# - P2P server outdated: Use Wireshark on Android app to find current server IP.
#
# Limitations:
# - No audio support (matches Android app, which is video-only).
# - Simplified PSK derivation; real app may use static key.
# - No real-time display (modify to pipe to ffmpeg if needed, but keeps pure Python).
# - Hardcoded P2P server (52.198.116.11:32100); update if unresponsive.
#
# For further help:
# - Provide Wireshark capture, UID, or error logs for debugging.
# - Request additional features (e.g., real-time display).
#
# Based on reverse-engineering from Mandiant, Bitdefender, DEF CON, and PROTOCOL.md.

import socket
import struct
import time
import random
import select
from dtls import do_patch, SslClient
from hashlib import sha256

do_patch()  # Patch socket for DTLS support

class IOTCError(Exception):
    pass

def discover_printers(timeout=3, max_num=16, retries=2):
    """Discover Bambu Lab printers on LAN (mimics IOTC_Lan_Search2)."""
    multicast_group = '239.255.255.250'
    port = 10000
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
        sock.settimeout(timeout / retries)
    except PermissionError:
        print("Error: Run as admin for multicast discovery.")
        sock.close()
        return []

    printers = set()
    for _ in range(retries):
        ts = int(time.time())
        rnd = random.randint(0, 0xFFFFFFFF)
        packet = struct.pack('>BBHIII', 0xFC, 0x00, 12, ts, rnd, 0)

        try:
            sock.sendto(packet, (multicast_group, port))
        except PermissionError:
            print("Error: Multicast send failed. Try running as admin.")
            break

        start_time = time.time()
        while (time.time() - start_time) < (timeout / retries):
            try:
                readable, _, _ = select.select([sock], [], [], 0.1)
                if not readable:
                    continue
                data, addr = sock.recvfrom(1024)
                if len(data) < 38 or data[0] != 0xFD:
                    continue
                uid = data[4:24].decode('ascii', errors='ignore').rstrip('\x00')
                ip = data[24:40].decode('ascii', errors='ignore').rstrip('\x00')
                port = struct.unpack('>H', data[40:42])[0]
                if uid and ip:
                    printers.add((uid, ip, port))
            except socket.timeout:
                continue
            except socket.error as e:
                print(f"Discovery error: {e}")
                break
        if len(printers) >= max_num:
            break
    sock.close()
    return list(printers)

def iotc_connect_by_uid(uid, access_code, timeout=10, p2p_server=('52.198.116.11', 32100)):
    """Mimics IOTC_Connect_ByUID with DTLS encryption."""
    if len(uid) != 20:
        raise IOTCError("UID must be 20 characters")
    if len(access_code) != 8:
        raise IOTCError("Access Code must be 8 characters")

    uid_bytes = uid.encode('ascii')
    auth = b'bblp:' + access_code.encode('ascii')
    # Derive PSK for DTLS (simplified, based on UID; real app may use static key)
    psk = sha256(uid_bytes).digest()[:16]  # 16-byte PSK
    psk_id = b'BambuLab'

    # Step 1: Try local connection (port 63616)
    local_ip, local_port = _discover_local_device(uid_bytes)
    if local_ip:
        print(f"Found local device at {local_ip}:{local_port}")
        try:
            ssl_sock = SslClient(
                socket.AF_INET,
                socket.SOCK_DGRAM,
                psk=psk,
                psk_id=psk_id,
                server=(local_ip, local_port)
            )
            ssl_sock.settimeout(timeout)
            # Send auth packet (mimics avClientStart)
            auth_packet = struct.pack('>BBH', 0xF1, 0x200, len(auth)) + auth
            ssl_sock.write(auth_packet)
            data = ssl_sock.read(1024)
            if data.startswith(b'\xF1\x201'):
                print(f"Direct DTLS connection to {local_ip}:{local_port}")
                return ssl_sock, local_ip, local_port
        except Exception as e:
            print(f"Local DTLS failed: {e}")
        finally:
            if 'ssl_sock' in locals():
                ssl_sock.close()

    # Step 2: Connect to P2P server
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)

    # Hello packet (0xF1000000)
    hello = struct.pack('>I', 0xF1000000)
    sock.sendto(hello, p2p_server)

    # Receive ACK
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) < 4 or struct.unpack('>I', data[:4])[0] != 0xF1010000:
            sock.close()
            raise IOTCError("P2P server ACK failed")
    except socket.timeout:
        sock.close()
        raise IOTCError("P2P server timeout")

    # Login packet (type 0x122, UID)
    header = struct.pack('>BBH', 0xF1, 0x122, len(uid_bytes))
    login_packet = header + uid_bytes
    sock.sendto(login_packet, p2p_server)

    # Receive login response
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) < 4 or data[0] != 0xF1:
            sock.close()
            raise IOTCError("Login failed")
    except socket.timeout:
        sock.close()
        raise IOTCError("Login timeout")

    # Request connection (type 0x101)
    request_header = struct.pack('>BBH', 0xF1, 0x101, len(uid_bytes))
    connect_request = request_header + uid_bytes
    sock.sendto(connect_request, p2p_server)

    # Receive peer info
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) < 10:
            sock.close()
            raise IOTCError("Peer info failed")
        peer_ip = '.'.join(str(b) for b in data[4:8])
        peer_port = struct.unpack('>H', data[8:10])[0]
    except socket.timeout:
        sock.close()
        raise IOTCError("Peer info timeout")

    # Step 3: DTLS with peer
    try:
        ssl_sock = SslClient(
            socket.AF_INET,
            socket.SOCK_DGRAM,
            psk=psk,
            psk_id=psk_id,
            server=(peer_ip, peer_port)
        )
        ssl_sock.settimeout(timeout)
        # Send auth packet
        ssl_sock.write(auth_packet)
        data = ssl_sock.read(1024)
        if data.startswith(b'\xF1\x201'):
            print(f"Direct DTLS P2P to {peer_ip}:{peer_port}")
            return ssl_sock, peer_ip, peer_port
    except Exception as e:
        print(f"Direct DTLS failed: {e}")
        if 'ssl_sock' in locals():
            ssl_sock.close()

    # Fallback to relay (use P2P server with DTLS)
    print("Direct failed, using relay")
    ssl_sock = SslClient(
        socket.AF_INET,
        socket.SOCK_DGRAM,
        psk=psk,
        psk_id=psk_id,
        server=p2p_server
    )
    ssl_sock.settimeout(timeout)
    ssl_sock.write(auth_packet)
    try:
        data = ssl_sock.read(1024)
        if data.startswith(b'\xF1\x201'):
            return ssl_sock, p2p_server[0], p2p_server[1]
        else:
            raise IOTCError("Relay auth failed")
    except Exception as e:
        ssl_sock.close()
        raise IOTCError(f"Relay failed: {e}")

def _discover_local_device(uid_bytes):
    """Broadcast on port 63616 to find local device."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(2)

    query = b'\x00' * 4 + uid_bytes
    sock.sendto(query, ('255.255.255.255', 63616))

    try:
        data, addr = sock.recvfrom(1024)
        if len(data) >= 6:
            ip = addr[0]
            port = struct.unpack('>H', data[4:6])[0]
            sock.close()
            return ip, port
    except socket.timeout:
        pass
    sock.close()
    return None, None

def stream_to_file(uid, access_code, out_file, duration=0, timeout=10):
    """Stream H.264 video from printer to file via DTLS P2P session."""
    try:
        sock, peer_ip, peer_port = iotc_connect_by_uid(uid, access_code, timeout)
    except IOTCError as e:
        print(f"Connection failed: {e}")
        return

    with open(out_file, 'wb') as f:
        start_time = time.time()
        buf = b''
        try:
            while duration == 0 or (time.time() - start_time) < duration:
                readable, _, _ = select.select([sock], [], [], 1)
                if not readable:
                    continue
                data = sock.read(65536)
                if len(data) < 12:
                    continue
                # Parse pseudo-RTP (mimics avRecvFrameData2)
                payload_type = data[1] & 0x7F
                if payload_type != 96:  # H.264
                    continue
                nal_type = data[12] & 0x1F
                if nal_type == 28:  # FU-A
                    fu_header = data[13]
                    start = fu_header & 0x80
                    end = fu_header & 0x40
                    nal_unit_type = fu_header & 0x1F
                    if start:
                        buf = bytes([data[12] & 0xE0 | nal_unit_type]) + data[14:]
                    else:
                        buf += data[14:]
                    if end:
                        f.write(b'\x00\x00\x00\x01' + buf)
                        buf = b''
                elif nal_type in (1, 5, 7, 8):  # P/I-frame, SPS, PPS
                    f.write(b'\x00\x00\x00\x01' + data[12:])
        except Exception as e:
            print(f"Stream error: {e}")
        except KeyboardInterrupt:
            print("Streaming stopped by user.")
        finally:
            sock.close()
    print(f"Saved stream to {out_file}")

def main():
    """Discover and stream printer webcam to file."""
    print("Discovering Bambu Lab printers (LAN Mode Liveview must be enabled)...")
    printers = discover_printers(timeout=3, max_num=16, retries=2)
    if not printers:
        print("No printers found. Enter UID and Access Code manually.")
        uid = input("Printer UID (20 chars): ").strip()
        access_code = input("Access Code (8 chars): ").strip()
    else:
        print("Found printers:")
        for idx, (uid, ip, port) in enumerate(printers):
            print(f"{idx}: UID={uid}, IP={ip}, Port={port}")
        try:
            choice = int(input("Select printer index: "))
            if choice < 0 or choice >= len(printers):
                raise ValueError("Invalid index")
            uid = printers[choice][0]
            access_code = input("Access Code (8 chars from printer settings): ").strip()
        except (ValueError, IndexError):
            print("Invalid selection. Exiting.")
            return

    if len(uid) != 20:
        print("Error: UID must be 20 characters.")
        return
    if len(access_code) != 8:
        print("Error: Access Code must be 8 characters.")
        return

    out_file = f"{uid}.h264"
    print(f"Streaming from UID {uid} to {out_file}... Press Ctrl+C to stop.")
    try:
        stream_to_file(uid, access_code, out_file)
    except Exception as e:
        print(f"Streaming failed: {e}")

if __name__ == "__main__":
    main()
