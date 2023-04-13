import openai
import serial
import sys
import glob
def serial_ports(): #pros steal functions from stack overflow
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

print("Available serial ports: ")
for x in range(0, len(serial_ports())):
    print(x, serial_ports()[x])
print("Enter your selection:")
port = input()
ser = serial.Serial(serial_ports()[int(port)], 9600)
ser.write(b'ack') #ack connection
openai.api_key = "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"
messages = [ {"role": "system", "content": 
              "You are a intelligent assistant."} ]
while True:
    message = ser.readline()
    message = message.decode('utf-8')
    print(message)
    if message:
        messages.append(
            {"role": "user", "content": message},
        )
        chat = openai.ChatCompletion.create(
            model="gpt-3.5-turbo", messages=messages
        )
    reply = chat.choices[0].message.content
    print(f"ChatGPT: {reply}")
    messages.append({"role": "assistant", "content": reply})
    chunk_size = 63 #The buffer is only 64 characters on the calculator. We must break the message down into 64 byte increments to avoid issues.
    for i in range(0, len(reply), chunk_size):
        chunk = reply[i:i+chunk_size]
        ser.write(bytes(chunk, 'utf-8'))