import os
import socket
import tqdm

def main():


    os.system(f"taskset -p -c 3 {os.getpid()}")

    while True:
        server_ip = "0.0.0.0"
        port = 5003
        buffer_size = 4096
        sep = "<SEPARATOR>"

        s = socket.socket()
        s.bind((server_ip, port))
        s.listen(999)
        print(f"[*] Listening as {server_ip}:{port}")
        client_socket, address = s.accept()
        print(f"[+] {address} is connected")

        recieved = client_socket.recv(buffer_size).decode()
        filename,filesize = recieved.split(sep)
        filename = os.path.basename(filename)
        filesize = int(filesize)

        progress = tqdm.tqdm(range(filesize), f"Receiving {filename}", unit="B", unit_scale=True, unit_divisor=1024)
        with open(filename, "wb") as file:
            while True:
                bytes_read = client_socket.recv(buffer_size)
                if not bytes_read: break
                file.write(bytes_read)
                progress.update(len(bytes_read))

        client_socket.close()
        s.close()

        if filename[-3:len(filename)] == ".py": 
            try: os.system(f"python {filename}")
            except: None

if __name__ == "__main__": 
    main()