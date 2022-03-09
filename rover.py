from machine import UART, Pin
import time
import _thread

class main():
    def __init__(self):
        time.sleep_ms(2000)
        self.connectionState = 'CLOSED'
        self.groundStationAddress = 101
        self.seqNum = 0
        self.ackNum = 0
        self.initLora()
        _thread.start_new_thread(self.connection, ())
    
        while True:
            time.sleep_ms(5000)

    def connection(self):
        while True:
            if self.connectionState == 'CLOSED':
                print('sent syn\n')
                self.msgTx('SYN')
                start = time.ticks_ms()
                self.connectionState = 'SYN-SENT'
            elif self.connectionState == 'SYN-SENT':
                data = self.parseMsg()
                if data and data[2] == 'SYN':
                    print('sent ack\n')
                    self.msgTx('ACK')
                    start = time.ticks_ms()
                    self.connectionState = 'ESTABLISHED'
                if time.ticks_diff(time.ticks_ms(), start) > 5000:
                    print('5 sec timeout\n')
                    self.msgTx('SYN')
                    start = time.ticks_ms()
            elif self.connectionState == 'ESTABLISHED':
                data = self.parseMsg()
                if data:
                    if data[2] == 'FIN':
                        self.connectionState = 'CLOSE-WAIT'
                    elif data[2] == 'ACK':
                        # send telemetry
                        print('sent ack\n')
                        self.msgTx('ACK')
                    elif data[2] == 'COM':
                        # parse command
                        print(''.join(data[3:]) + '\n')
                        print('sent ack\n')
                        self.msgTx('ACK')
                if time.ticks_diff(time.ticks_ms(), start) > 5000:
                    print('5 sec timeout\n')
                    self.msgTx('ACK')
                    start = time.ticks_ms()
            elif self.connectionState == 'CLOSE-WAIT':
                print('sent fin\n')
                self.msgTx('FIN')
                start = time.ticks_ms()
                self.connectionState = 'LAST-ACK'
            elif self.connectionState == 'LAST-ACK':
                data = self.parseMsg()
                if data and data[2] == 'ACK':
                    print('Connection successfully terminated\n')
                    self.connectionState = 'CLOSED'
                    self.seqNum = 0
                    self.ackNum = 0
                    time.sleep_ms(3000)
                if time.ticks_diff(time.ticks_ms(), start) > 5000:
                    print('5 sec timeout\n')
                    self.msgTx('FIN')
                    start = time.ticks_ms()

    def initLora(self):
        self.uart = UART(1,baudrate=115200, tx=Pin(4), rx=Pin(5), bits=8, parity=None, stop=1, timeout=2)
        self.uart.write('AT+NETWORKID=5\r\n')
        if self.readLora() != '+OK':
            print('Error')
        self.uart.write('AT+ADDRESS=102\r\n')
        if self.readLora() != '+OK':
            print('Error')

    def readLora(self):
        start = time.ticks_ms()
        while time.ticks_diff(time.ticks_ms(), start) < 5000:
            line = self.read()
            if line:
                return line
        return 'Error'

    def msgTx(self, c):
        msg = str(self.seqNum) + ' ' + str(self.ackNum) + ' ' + c
        self.send('AT+SEND=' + str(self.groundStationAddress) + ',' + str(len(msg)) + ',' + msg + '\r\n')

    def parseMsg(self):
        data = self.msgRx(self.read())
        if data and (' ' in data):
            data = data.split(' ')
            return data

    def msgRx(self, msg):
        if msg and msg[:5] == '+RCV=':
            msg = msg[5:]
            self.message = msg.split(',')
            return self.message[2]
    
    def read(self):
        if self.uart.any():
            return self.uart.readline().decode().strip('\r\n')

    def send(self, msg):
        self.uart.write(msg.encode('ascii'))

if __name__=="__main__":
    main()