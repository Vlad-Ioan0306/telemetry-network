import socket
import struct
import sys
from datetime import datetime
import polars
import duckdb

# Define Labels
MEM_LABELS = {
    0: "Total Mem",
    1: "Mem Free",
    2: "Mem available",
    3: "Buffers",
    4: "Cached",
}
LOAD_FIELDS = [
    "Load 1m",
    "Load 5m",
    "Load 15m",
    "Processes running",
    "Total Processes",
    "Last_PID",
]

# Setup socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# This line fixes the [Errno 98] error:
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("0.0.0.0", 8080))

print("👂 Listening for data on 8080...")

try:
    while True:
        data, addr = sock.recvfrom(1024)
        if len(data) < 57:
            continue

        # Unpack
        ts_raw = struct.unpack(">Q", data[:8])[0]
        dt_object = datetime.fromtimestamp(ts_raw)
        ts = dt_object.strftime("%Y/%m/%d||%H:%M:%S")
        meminfo_raw = list(struct.iter_unpack(">HI", data[8:38]))
        loadavg_raw = struct.unpack(">IIIBHI", data[38:57])
        loadavg_fields = [val / 100.0 for val in loadavg_raw[:3]]
        loadavg_total = loadavg_fields + list(loadavg_raw[3::])

        # Print with labels
        print(f"\n--- Packet from {addr} ---")
        print(f"Timestamp: {ts}")

        for idx, (h, i) in enumerate(meminfo_raw):
            label = MEM_LABELS.get(idx, "Unknown")
            print(f"{label}: {i}")

        load_dict = dict(zip(LOAD_FIELDS, loadavg_total))
        for key, val in load_dict.items():
            print(f"{key}: {val}")

except KeyboardInterrupt:
    print("\n🛑 Stopping...")
    sock.close()
    sys.exit()
