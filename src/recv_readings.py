import socket
import struct
import sys
from datetime import datetime
import polars as pl
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

def main() -> None:
    # Setup socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # This line fixes the [Errno 98] error:
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", 8080))

    print("👂 Listening for data on 8080...")

    database = duckdb.connect("data.db")
    tables = database.execute(
        "SELECT table_name FROM information_schema.tables WHERE table_schema = 'main'"
    ).fetchall()

    for (table_name,) in tables:
        # Validate table_name is a safe identifier before interpolating
        if not table_name.replace("_", "").isalnum():
            print(f"Skipping unsafe table name: {table_name}")
            continue
        database.execute(f"DROP TABLE IF EXISTS {table_name}")
        print(f"Dropped {table_name}")

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
            # print(f"\n--- Packet from {addr} ---")
            # print(f"Timestamp: {ts}")

            MEM_FIELDS = []
            for idx, (h, i) in enumerate(meminfo_raw):
                label = MEM_LABELS.get(idx, "Unknown")
                if label != "Unknown":
                    MEM_FIELDS.append(label)

            # Create dictionaries
            # Ensure meminfo_raw values align with MEM_FIELDS
            mem_values = [i for h, i in meminfo_raw if MEM_LABELS.get(h) != "Unknown"]
            meminfo_dict = dict(zip(MEM_FIELDS, mem_values))

            load_dict = dict(zip(LOAD_FIELDS, loadavg_total))

            # Create DataFrames
            meminfo_df = pl.DataFrame(meminfo_dict)
            loadavg_df = pl.DataFrame(load_dict)

            # Add timestamp
            meminfo_df = meminfo_df.with_columns(pl.lit(ts).alias("timestamp"))
            loadavg_df = loadavg_df.with_columns(pl.lit(ts).alias("timestamp"))

            # --- KEY CHANGE START ---

            # 1. Create table structure from the first DataFrame (if it doesn't exist)
            # LIMIT 0 ensures no data is inserted yet, just schema creation
            database.execute(
                "CREATE TABLE IF NOT EXISTS meminfo_data AS SELECT * FROM meminfo_df LIMIT 0"
            )
            database.execute(
                "CREATE TABLE IF NOT EXISTS loadavg_data AS SELECT * FROM loadavg_df LIMIT 0"
            )

            # Optional: Create combined table schema dynamically too
            # We join them in Polars first to get the combined schema
            combined_df = meminfo_df.join(loadavg_df, on="timestamp", how="left")
            database.execute(
                "CREATE TABLE IF NOT EXISTS telemetry_combined AS SELECT * FROM combined_df LIMIT 0"
            )

            # 2. Append the actual data
            database.execute("INSERT INTO meminfo_data SELECT * FROM meminfo_df")
            database.execute("INSERT INTO loadavg_data SELECT * FROM loadavg_df")
            database.execute("INSERT INTO telemetry_combined SELECT * FROM combined_df")
    except KeyboardInterrupt:
        print("\n🛑 Stopping...")
        database.close()
        sock.close()
        sys.exit()

    database.close()


if __name__ == "__main__":
    main()
