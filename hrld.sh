#!/bin/bash

# ==============================================
# Hot Reload Script for C Projects
# ==============================================

# ------------------------------
# Configuration Variables
# ------------------------------

# Terminal color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ------------------------------
# Argument Parsing
# ------------------------------

# Check for project argument
if [ -z "$1" ]; then
    echo -e "${RED}[ERROR] No project name specified.${NC}"
    echo -e "${YELLOW}Usage: ./hrld.sh <project_name>${NC}"
    exit 1
fi

PROJECT_NAME="$1"

# ------------------------------
# Build and Run Commands
# ------------------------------

# Define build command (modify according to your project structure)
BUILD_CMD="make $PROJECT_NAME"

# Define run command
RUN_CMD="./$PROJECT_NAME"

# Define log file
LOG_FILE="hot_reload_${PROJECT_NAME}.log"

# ------------------------------
# Initialize Variables
# ------------------------------

# PID of the running server
SERVER_PID=0

# ------------------------------
# Function Definitions
# ------------------------------

# Function to log messages with color
log() {
    local level="$1"
    local message="$2"
    case "$level" in
        INFO)
            echo -e "${GREEN}$(date +"%Y-%m-%d %H:%M:%S") [INFO] ${message}${NC}" | tee -a "$LOG_FILE"
            ;;
        WARN)
            echo -e "${YELLOW}$(date +"%Y-%m-%d %H:%M:%S") [WARN] ${message}${NC}" | tee -a "$LOG_FILE"
            ;;
        ERROR)
            echo -e "${RED}$(date +"%Y-%m-%d %H:%M:%S") [ERROR] ${message}${NC}" | tee -a "$LOG_FILE"
            ;;
        DEBUG)
            echo -e "${CYAN}$(date +"%Y-%m-%d %H:%M:%S") [DEBUG] ${message}${NC}" | tee -a "$LOG_FILE"
            ;;
        *)
            echo -e "$(date +"%Y-%m-%d %H:%M:%S") [UNKNOWN] ${message}" | tee -a "$LOG_FILE"
            ;;
    esac
}

# Function to build the project
build_project() {
    log "INFO" "Starting build for project '$PROJECT_NAME'..."
    $BUILD_CMD 2>&1 | tee -a "$LOG_FILE"
    if [ $? -eq 0 ]; then
        log "INFO" "Build successful."
        return 0
    else
        log "ERROR" "Build failed."
        return 1
    fi
}

# Function to run the project
run_project() {
    log "INFO" "Starting project '$PROJECT_NAME'..."
    $RUN_CMD &
    SERVER_PID=$!
    log "INFO" "Project '$PROJECT_NAME' started with PID $SERVER_PID."
}

# Function to stop the project
stop_project() {
    if [ $SERVER_PID -ne 0 ]; then
        log "WARN" "Stopping project '$PROJECT_NAME' with PID $SERVER_PID..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null
        log "INFO" "Project '$PROJECT_NAME' stopped."
        SERVER_PID=0
    fi
}

# Function to handle cleanup on exit
cleanup() {
    log "WARN" "Cleaning up before exit..."
    stop_project
    exit 0
}

# Function to handle build and run
build_and_run() {
    if build_project; then
        stop_project
        run_project
    else
        log "ERROR" "Rebuild failed. Server not restarted."
    fi
}

# Function to debounce changes
debounce_changes() {
    sleep 1
}

# ------------------------------
# Trap Signals for Cleanup
# ------------------------------

trap cleanup SIGINT SIGTERM

# ------------------------------
# Initial Build and Run
# ------------------------------

build_and_run

# ------------------------------
# Define Files to Watch
# ------------------------------

# Find all .c and .h files containing the project name
WATCH_FILES=$(find . -type f \( -name "*$PROJECT_NAME*.c" -o -name "*$PROJECT_NAME*.h" \))

if [ -z "$WATCH_FILES" ]; then
    log "ERROR" "No source files found for project '$PROJECT_NAME'. Exiting."
    exit 1
fi

# Extract unique directories to watch
WATCH_DIRS=$(echo "$WATCH_FILES" | xargs -n1 dirname | sort | uniq)

# ------------------------------
# Monitoring Loop
# ------------------------------

log "INFO" "Watching for changes in project '$PROJECT_NAME' source files..."

# Monitor file changes using inotifywait
inotifywait -m -e modify,create,delete --format '%w%f' -r $WATCH_DIRS | while read MODIFIED_FILE
do
    # Check if the modified file is relevant
    if [[ "$MODIFIED_FILE" == *"$PROJECT_NAME"* && ( "$MODIFIED_FILE" == *.c || "$MODIFIED_FILE" == *.h ) ]]; then
        log "INFO" "Change detected in '$MODIFIED_FILE'. Rebuilding..."
        build_and_run
        debounce_changes
    fi
done
