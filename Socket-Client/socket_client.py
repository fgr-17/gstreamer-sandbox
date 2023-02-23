#!/usr/bin/env python

import socket, time

HOST = "localhost"  # The server's hostname or IP address
PORT = 4007  # The port used by the server

print("**************************************")
print("******* Socket Client Tester *********")
print("**************************************")

def socket_read(sock, expected):
    """Read expected number of bytes from sock
    Will repeatedly call recv until all expected data is received
    """
    buffer = b''
    while len(buffer) < expected:
        print("recibido 1")
        buffer += sock.recv(expected - len(buffer))
    return buffer

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print(f"Connected to port:", PORT)
    while 1:
        #First Receive the frame number
        raw_frame = s.recv(4)
        frame = int.from_bytes(raw_frame, byteorder="little")
        #Second Receive the frame size
        raw_size = s.recv(4)
        size = int.from_bytes(raw_size, byteorder="little") 
        #Third Receive the frame
        raw_img = s.recv(size)
        # Open a file, save img and close it
        f = open (str(frame)+".jpg", "wb")
        f.write(raw_img)
        f.close()
        if raw_frame != 0 :
            print(f"Rcv Frame: {frame} - with lenght: {size}")
        time.sleep(0.1)
