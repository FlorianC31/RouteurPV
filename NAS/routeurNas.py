import serial

ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)

while True:
    data = ser.readline().decode('utf-8').strip()
    print(data)