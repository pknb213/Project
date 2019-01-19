## server.py

import socket

def run_server(port=4000):
    host = ''
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen(1)
        conn, addr = s.accept()
        while True:
            msg = conn.recv(1024)
            if not data: break
            print(msg.decode())
            conn.senall(msg)
        conn.close()

if __name__ == '__main__':
    run_server()


while True:
	if client_socket.recv(1029):
		print("case 1")
		data = client_socket.recv(1029)
	else :
		print("case 2")
		data = client_socket.sendall(str)
	if not data: break
	print(data)
	time.sleep(2)


# i2c.py
'''
import os
import fcntl
import time

I2C_SLAVE               = 0x703
BH1750                  = 0x23
CONTINUOUS_HIGH_RES_MOD = 0x11
RESET_MODE              = 0x07
ONE_TIME_RES_MODE2      = 0x21

fd = os.open('/dev/i2c-1', os.O_RDWR)
fcntl.ioctl(fd, I2C_SLAVE, BH1750)
r = os.write(fd, bytes([CONTINUOUS_HIGH_RES_MOD]))

try:
    while True:
        data = os.read(fd, 2)
        print(int(data[0]) * 256 + int(data[1]))
        time.sleep(0.5)
except KeyboardInterrupt:
    os.close(fd)
'''

'''
    1) 모드 1byte 값을 조도 센서에 보내 동작 모드를 설정
    2) 조도 센서로부터 측정한 조도 값을 word로 (2byte)로 읽어야한다
    3) SMBus에서 제공하는 write_byte(addr,value) 사용하여 1byte로 전송이 가능하지만
        word로 data를 읽어들이는 read_word_data(addr,cmd)로 cmd를 먼저 보내고 2byte를 읽는 함수를 지원

    참고로 BH1750은 첫 byte가 MSB인데 read_word_date() 함수는 이를 LSB로 읽어 LSB와 MSB를 바꿈
'''

'''
from smbus import SMBus
import time

BH1750 = 0x23
CONTINUOUS_HIGH_RES_MOD = 0x11
ONE_TIME_HIGH_RES_MODE_1 = 0x20
ONE_TIME_HIGH_RES_MODE_2 = 0x21

bus = SMBus(1)

while True:
    word = bus.read_word_data(BH1750, CONTINUOUS_HIGH_RES_MOD)
    swap = (word & 0xff) * 256 + (word >> 8)
    print(swap)
    time.sleep(0.2)
'''
