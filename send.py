import socket
import tqdm
import os

def send(filename):
    sep = "<SEPARATOR>"
    buffer_size = 4096
    mac_ip = "192.168.68.118"
    filesize = os.path.getsize(filename)
    port = 5003

    s = socket.socket()
    print(f"[+] Connecting to {mac_ip}:{port}")
    s.connect((mac_ip, port))
    print("[+] Connected")

    s.send(f"{filename}{sep}{filesize}".encode())

    progress = tqdm.tqdm(range(filesize), f"Sending {filename}", unit="B", unit_scale=True, unit_divisor=1024)
    with open(filename, "rb") as file:
        while True:
            bytes_read = file.read(buffer_size)
            if not bytes_read: break
            s.sendall(bytes_read)
            progress.update(len(bytes_read))

    s.close()
