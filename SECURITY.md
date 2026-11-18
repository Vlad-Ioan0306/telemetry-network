# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.0   | :white_check_mark: |

## Reporting a Vulnerability

This project is a small educational telemetry collector. If you find a security issue, open an issue or contact the repository owner directly. There is no formal security response process or SLA.

## Known Limitations

- **No encryption**: Telemetry packets are sent over UDP in plaintext. Anyone on the same network can capture and read system load and memory data.
- **No authentication**: The receiver accepts packets from any source IP. There is no sender verification.
- **No integrity protection**: Packets can be modified in transit without detection.
- **UDP unreliability**: Packets may be dropped, reordered, or duplicated with no notification.
- **Local network only**: Designed for LAN use. Exposing UDP 8080 to the public internet is not recommended.
- **`/proc` access**: The C sender reads `/proc/loadavg` and `/proc/meminfo`, which may be restricted in containerized or hardened environments.

## Recommendations

- Run only on trusted local networks.
- Restrict UDP 8080 via firewall to trusted source IPs.
- Do not send sensitive telemetry over untrusted networks.
- Consider adding TLS/DTLS, sender authentication, or packet signing if adapting this for production use.
