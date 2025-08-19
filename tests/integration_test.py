
#!/usr/bin/env python3
"""
Integration tests for the IOTC library using the mock server.

This script starts the mock server, runs various integration tests,
and validates the IOTC library behavior in realistic scenarios.
"""

import subprocess
import time
import socket
import threading
import sys
import os
import signal
from pathlib import Path

class MockServerManager:
    def __init__(self, port=8080):
        self.port = port
        self.process = None
        self.server_ready = False
    
    def start(self):
        """Start the mock server"""
        print(f"Starting mock server on port {self.port}...")
        
        # Build mock server if needed
        mock_server_path = Path("tests/mock_iotc_server")
        if not mock_server_path.exists():
            print("Building mock server...")
            result = subprocess.run([
                "gcc", "-o", str(mock_server_path),
                "tests/mock_iotc_server.c",
                "-pthread"
            ], capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Failed to build mock server: {result.stderr}")
                return False
        
        # Start server
        self.process = subprocess.Popen([
            str(mock_server_path), "-p", str(self.port), "-v"
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        # Wait for server to be ready
        for _ in range(30):  # Wait up to 3 seconds
            if self._check_server_ready():
                self.server_ready = True
                print("Mock server is ready")
                return True
            time.sleep(0.1)
        
        print("Mock server failed to start")
        self.stop()
        return False
    
    def stop(self):
        """Stop the mock server"""
        if self.process:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.process = None
        self.server_ready = False
    
    def _check_server_ready(self):
        """Check if server is accepting connections"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex(('localhost', self.port))
            sock.close()
            return result == 0
        except:
            return False

def test_device_registration(port):
    """Test device registration with mock server"""
    print("Testing device registration...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', port))
        
        # Send device login
        test_uid = "TEST_DEVICE_12345678"
        sock.send(f"DEVICE_LOGIN:{test_uid}".encode())
        
        response = sock.recv(1024).decode()
        sock.close()
        
        if response == "IOTC_LOGIN_OK":
            print("✓ Device registration successful")
            return True
        else:
            print(f"✗ Device registration failed: {response}")
            return False
            
    except Exception as e:
        print(f"✗ Device registration error: {e}")
        return False

def test_session_request(port):
    """Test session request to registered device"""
    print("Testing session request...")
    
    try:
        # First register a device
        sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock1.connect(('localhost', port))
        test_uid = "TEST_DEVICE_87654321"
        sock1.send(f"DEVICE_LOGIN:{test_uid}".encode())
        response = sock1.recv(1024).decode()
        
        if response != "IOTC_LOGIN_OK":
            print(f"✗ Failed to register device for session test")
            sock1.close()
            return False
        
        # Now request session
        sock2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock2.connect(('localhost', port))
        sock2.send(f"SESSION_REQUEST:{test_uid}".encode())
        
        response = sock2.recv(1024).decode()
        sock1.close()
        sock2.close()
        
        if response.startswith("IOTC_SESSION_OK"):
            print("✓ Session request successful")
            return True
        else:
            print(f"✗ Session request failed: {response}")
            return False
            
    except Exception as e:
        print(f"✗ Session request error: {e}")
        return False

def test_heartbeat(port):
    """Test heartbeat functionality"""
    print("Testing heartbeat...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', port))
        
        sock.send(b"HEARTBEAT")
        response = sock.recv(1024).decode()
        sock.close()
        
        if response == "IOTC_HEARTBEAT_OK":
            print("✓ Heartbeat successful")
            return True
        else:
            print(f"✗ Heartbeat failed: {response}")
            return False
            
    except Exception as e:
        print(f"✗ Heartbeat error: {e}")
        return False

def test_invalid_command(port):
    """Test invalid command handling"""
    print("Testing invalid command handling...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('localhost', port))
        
        sock.send(b"INVALID_COMMAND")
        response = sock.recv(1024).decode()
        sock.close()
        
        if response == "IOTC_ER_INVALID_COMMAND":
            print("✓ Invalid command handled correctly")
            return True
        else:
            print(f"✗ Invalid command handling failed: {response}")
            return False
            
    except Exception as e:
        print(f"✗ Invalid command test error: {e}")
        return False

def test_library_compilation():
    """Test that the IOTC library compiles correctly"""
    print("Testing IOTC library compilation...")
    
    try:
        result = subprocess.run(["make", "clean"], capture_output=True, text=True)
        result = subprocess.run(["make"], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ Library compilation successful")
            return True
        else:
            print(f"✗ Library compilation failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"✗ Compilation test error: {e}")
        return False

def test_library_unit_tests():
    """Run the comprehensive unit tests"""
    print("Running comprehensive unit tests...")
    
    try:
        # Build test runner
        result = subprocess.run([
            "gcc", "-o", "test_runner",
            "tests/test_libIOTCAPIsT.c",
            "native/libIOTCAPIsT.c",
            "-I", "native/include",
            "-pthread"
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"✗ Failed to build test runner: {result.stderr}")
            return False
        
        # Run tests
        result = subprocess.run(["./test_runner"], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✓ Unit tests passed")
            # Print test output for verification
            print(result.stdout)
            return True
        else:
            print(f"✗ Unit tests failed: {result.stderr}")
            print(result.stdout)
            return False
            
    except Exception as e:
        print(f"✗ Unit test error: {e}")
        return False
    finally:
        # Clean up
        if os.path.exists("test_runner"):
            os.remove("test_runner")

def run_stress_test(port, duration=10):
    """Run stress test with multiple concurrent connections"""
    print(f"Running stress test for {duration} seconds...")
    
    results = {"success": 0, "failed": 0}
    start_time = time.time()
    
    def worker():
        while time.time() - start_time < duration:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1)
                sock.connect(('localhost', port))
                
                sock.send(b"HEARTBEAT")
                response = sock.recv(1024).decode()
                sock.close()
                
                if response == "IOTC_HEARTBEAT_OK":
                    results["success"] += 1
                else:
                    results["failed"] += 1
                    
            except:
                results["failed"] += 1
            
            time.sleep(0.01)  # Small delay between requests
    
    # Start multiple worker threads
    threads = []
    for _ in range(5):
        thread = threading.Thread(target=worker)
        threads.append(thread)
        thread.start()
    
    # Wait for completion
    for thread in threads:
        thread.join()
    
    total = results["success"] + results["failed"]
    success_rate = (results["success"] / total * 100) if total > 0 else 0
    
    print(f"Stress test completed: {results['success']}/{total} requests successful ({success_rate:.1f}%)")
    
    return success_rate > 95  # Consider success if >95% requests succeeded

def main():
    print("IOTC Library Integration Test Suite")
    print("===================================")
    
    # Test library compilation first
    if not test_library_compilation():
        print("❌ Library compilation failed, cannot continue")
        return 1
    
    # Run unit tests
    if not test_library_unit_tests():
        print("❌ Unit tests failed")
        return 1
    
    # Start mock server
    server = MockServerManager(port=8080)
    
    try:
        if not server.start():
            print("❌ Failed to start mock server")
            return 1
        
        # Run integration tests
        tests = [
            test_device_registration,
            test_session_request,
            test_heartbeat,
            test_invalid_command,
            lambda port: run_stress_test(port, 5)  # 5 second stress test
        ]
        
        passed = 0
        total = len(tests)
        
        for test in tests:
            if test(server.port):
                passed += 1
            time.sleep(0.5)  # Small delay between tests
        
        print(f"\nIntegration test results: {passed}/{total} tests passed")
        
        if passed == total:
            print("🎉 All integration tests passed!")
            return 0
        else:
            print("❌ Some integration tests failed")
            return 1
    
    finally:
        server.stop()
        print("Mock server stopped")

if __name__ == "__main__":
    sys.exit(main())
