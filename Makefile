CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -Iinclude
SRCDIR = src
BUILDDIR = build

# Source files
COMMON_SOURCES = $(SRCDIR)/crc32.cpp
SERVER_SOURCES = $(SRCDIR)/server.cpp
CLIENT_SOURCES = $(SRCDIR)/client.cpp

# Targets
all: $(BUILDDIR)/telemetry_server $(BUILDDIR)/telemetry_client

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/telemetry_server: $(SERVER_SOURCES) $(COMMON_SOURCES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_SOURCES) $(COMMON_SOURCES)

$(BUILDDIR)/telemetry_client: $(CLIENT_SOURCES) $(COMMON_SOURCES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_SOURCES) $(COMMON_SOURCES)

clean:
	rm -rf $(BUILDDIR)

test: all
	@echo "Starting Reliable Telemetry System Test"
	@echo "======================================"
	@echo "Starting client..."
	@cd $(BUILDDIR) && ./telemetry_client &
	@CLIENT_PID=$$!; \
	sleep 1; \
	echo "Starting server..."; \
	cd $(BUILDDIR) && ./telemetry_server & \
	SERVER_PID=$$!; \
	sleep 10; \
	echo "Stopping processes..."; \
	kill $$SERVER_PID $$CLIENT_PID 2>/dev/null || true; \
	wait $$SERVER_PID $$CLIENT_PID 2>/dev/null || true
	@echo "Test complete!"

.PHONY: all clean test