# Telemetry Network

System telemetry collector: a C UDP sender reads `/proc/loadavg` and `/proc/meminfo`, serializes them into packed binary packets, and sends them to a Python UDP receiver that parses the packets and stores them in DuckDB tables via Polars DataFrames.

## Architecture

```
C sender (get_readings.c + structs.c)
  │ reads /proc/loadavg, /proc/meminfo
  │ serializes to packed binary packet (network byte order)
  ▼ UDP port 8080
Python receiver (recv_readings.py)
  │ unpacks packets, builds Polars DataFrames
  │ writes to DuckDB tables (meminfo_data, loadavg_data, telemetry_combined)
  ▼ data.db (DuckDB)
```

## Project Layout

```
telemetry_network/
├── pyproject.toml          # uv project config + dependencies
├── uv.lock                 # locked dependency versions
├── README.md               # this file
├── .python-version         # Python 3.14 pin
├── src/
│   ├── get_readings.c      # C sender: reads system telemetry, sends UDP packets
│   ├── structs.c           # C struct definitions + serialization helpers
│   ├── structs.h           # C struct type definitions
│   ├── recv_readings.py    # Python receiver: UDP listener, DuckDB writer
│   ├── run_all.py          # orchestrator: compile, run receiver, run sender, cleanup
│   ├── get_readings        # compiled C binary (gitignored)
│   └── data.db             # DuckDB database file (gitignored)
```

## Build & Run

### Install dependencies

```bash
# Requires uv
uv sync
```

### One-command run

```bash
cd src
python run_all.py
```

`run_all.py` detects your local IP, compiles the C sender, starts the receiver in the background, sends 60 telemetry packets (1 per second), then shuts down the receiver.

### Manual run

**Terminal 1 — receiver:**

```bash
cd src
uv run python recv_readings.py
```

**Terminal 2 — sender:**

```bash
cd src
./get_readings <receiver-ip>
```

Omit `<receiver-ip>` to send to `127.0.0.1` (localhost).

## Output

The receiver creates `src/data.db` (DuckDB) with three tables:

| Table | Columns |
|-------|---------|
| `meminfo_data` | Total Mem, Mem Free, Mem available, Buffers, Cached, timestamp |
| `loadavg_data` | Load 1m, Load 5m, Load 15m, Processes running, Total Processes, Last_PID, timestamp |
| `telemetry_combined` | all meminfo + loadavg columns joined on timestamp |

Each table receives one row per UDP packet (60 rows per full `run_all.py` run).

## Query the database

```bash
uv run python -c "import duckdb; con = duckdb.connect('src/data.db'); print(con.execute('SELECT * FROM loadavg_data LIMIT 5').fetchdf())"
```

## Requirements

- Python 3.14+
- `uv` for dependency management
- `gcc` (or any C compiler) for building the sender
- Linux (reads `/proc/loadavg` and `/proc/meminfo`)

## Networking

The receiver binds to `0.0.0.0:8080` (all interfaces). To send from another computer:

```bash
./get_readings <receiver-lan-ip>
```

Make sure UDP port 8080 is open in the receiver's firewall:

```bash
# firewalld
sudo firewall-cmd --add-port=8080/udp --permanent
sudo firewall-cmd --reload

# ufw
sudo ufw allow 8080/udp
```

## Notes

- UDP is fire-and-forget: no delivery guarantee, no retransmission, no encryption.
- The receiver drops packets smaller than 57 bytes.
- `data.db` and the compiled `get_readings` binary are gitignored.
