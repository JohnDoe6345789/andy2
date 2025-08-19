
CC=gcc
CFLAGS=-Wall -Wextra -fPIC -std=c99
LDFLAGS=-shared
TARGET=native/lib/libIOTCAPIsT.so
SOURCE=native/libIOTCAPIsT.c
HEADER=native/include/libIOTCAPIsT.h
OBJECT=native/libIOTCAPIsT.o
MOCK_SERVER=tests/mock_iotc_server
TEST_RUNNER=test_runner

.PHONY: all clean install test test-comprehensive test-integration test-mock-server android

all: $(TARGET)

$(TARGET): $(OBJECT)
	mkdir -p native/lib
	$(CC) $(LDFLAGS) -o $@ $<

$(OBJECT): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -I native/include -c $< -o $@

$(MOCK_SERVER): tests/mock_iotc_server.c
	$(CC) $(CFLAGS) -pthread -o $@ $<

clean:
	rm -f $(OBJECT) $(TARGET) $(TEST_RUNNER) $(MOCK_SERVER)
	rm -rf native/lib

install: $(TARGET)
	mkdir -p app/src/main/assets/native
	cp $(TARGET) app/src/main/assets/native/

# Basic unit tests (legacy)
test: $(TARGET)
	$(CC) -o $(TEST_RUNNER) tests/test_libIOTCAPIsT.c -L native/lib -lIOTCAPIsT -I native/include
	LD_LIBRARY_PATH=native/lib ./$(TEST_RUNNER)
	rm -f $(TEST_RUNNER)

# Comprehensive unit tests (new)
test-comprehensive: $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -pthread -o $(TEST_RUNNER) tests/test_libIOTCAPIsT.c $(SOURCE) -I native/include
	./$(TEST_RUNNER)
	rm -f $(TEST_RUNNER)

# Mock server for testing
test-mock-server: $(MOCK_SERVER)
	@echo "Starting mock IOTC server on port 8080"
	@echo "Use Ctrl+C to stop, or connect with: telnet localhost 8080"
	./$(MOCK_SERVER) -p 8080 -v

# Full integration tests
test-integration: $(TARGET) $(MOCK_SERVER)
	python3 tests/integration_test.py

# Run all tests
test-all: test-comprehensive test-integration
	@echo "All tests completed!"

android: install
	cd app && ./gradlew assembleDebug

.DEFAULT_GOAL := all
