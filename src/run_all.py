import socket
import subprocess
import time
import os
import signal


def get_local_ip():
    """Automatically finds the computer's local network IP address."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # We don't actually connect, but this triggers the OS to pick the right interface
        s.connect(("8.8.8.8", 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = "127.0.0.1"
    finally:
        s.close()
    return IP


def main() -> None:
    # 1. Get the local IP
    ip = get_local_ip()
    print(f"🚀 Detected Local IP: {ip}")

    # 2. Compile the C program
    print("🛠️  Compiling sender.c...")
    compile_res = subprocess.run(
        ["gcc", "get_readings.c", "structs.c", "-o", "get_readings"]
    )
    if compile_res.returncode != 0:
        print("❌ Compilation failed!")
        return

    # 3. Start the Python Receiver in the background
    print("👂 Starting receiver.py in background...")
    # We use Popen so the script doesn't stop and wait for the receiver to finish
    receiver_proc = subprocess.Popen(["python3", "recv_readings.py"])

    # Give the receiver a second to wake up and bind to the port
    time.sleep(1)

    try:
        # 4. Run the C Sender, passing the IP as an argument
        print(f"📤 Sending data to {ip}...")
        subprocess.run(["./get_readings", ip], check=True)

        # Give it a moment to finish sending before we shut down
        time.sleep(1)

    except KeyboardInterrupt:
        pass
    finally:
        # 5. Clean up: Kill the receiver process
        print("\n🛑 Shutting down receiver...")
        receiver_proc.terminate()
        receiver_proc.wait(timeout=5)
        print("Done!")


if __name__ == "__main__":
    main()
