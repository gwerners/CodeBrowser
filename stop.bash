#!/bin/bash
# CodeBrowser - Stop all running instances and free port 3000
#
# Usage:
#   ./stop.bash          # graceful kill
#   ./stop.bash force    # force kill (-9)

SIGNAL="-TERM"
if [[ "${1:-}" == "force" ]]; then
  SIGNAL="-9"
  echo "Force killing..."
fi

# Kill codebrowser and its child clangd processes
for proc in codebrowser clangd; do
  PIDS=$(pgrep -x "$proc" 2>/dev/null || true)
  if [[ -n "$PIDS" ]]; then
    echo "Killing $proc: $PIDS"
    kill $SIGNAL $PIDS 2>/dev/null || true
  fi
done

# Kill any stuck curl connections to port 3000
CURL_PIDS=$(lsof -i :3000 -t 2>/dev/null | sort -u)
if [[ -n "$CURL_PIDS" ]]; then
  echo "Killing processes on port 3000: $CURL_PIDS"
  kill $SIGNAL $CURL_PIDS 2>/dev/null || true
fi

sleep 1

# Verify port is free
REMAINING=$(lsof -i :3000 -t 2>/dev/null || true)
if [[ -n "$REMAINING" ]]; then
  echo "Port 3000 still in use, force killing: $REMAINING"
  kill -9 $REMAINING 2>/dev/null || true
  sleep 1
fi

# Final check
if lsof -i :3000 &>/dev/null; then
  echo "ERROR: Port 3000 still occupied!"
  lsof -i :3000
  exit 1
else
  echo "Port 3000 is free."
fi
