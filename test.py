import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('localhost',12345))
sock.listen(1)
sock.close()
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('localhost',12345))