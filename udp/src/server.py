import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 23333))

i = 0
try:
    while True:
        data, address = sock.recvfrom(size)
        sock.sendto("%s %s"%(i, data), address)
        i += 1
finally:
    sock.close()