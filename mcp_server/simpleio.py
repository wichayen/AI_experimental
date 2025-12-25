import usb.core
import usb.backend.libusb1
import usb.util
import binascii
import time


def usb_led(b, r, g, y):
    b = 0x1 if b != 0 else 0x0
    r = 0x1 if r != 0 else 0x0
    g = 0x1 if g != 0 else 0x0
    y = 0x1 if y != 0 else 0x0    
    
    # Specify the path to the libusb-1.0.dll
    usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-x64/dll/libusb-1.0.dll"))
    # usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-Win32/dll/libusb-1.0.dll"))

    # Find the USB device based on its vendor and product IDs
    vendor_id = 0xCAFE# Replace with your device's vendor ID
    product_id = 0x0300  # Replace with your device's product ID

    # Find the device
    device = usb.core.find(idVendor=vendor_id, idProduct=product_id)
    if device is None:
        raise ValueError('Device not found')
#     else:
#     #     print(device)
#         print('found device')

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    device.set_configuration()

    # Find the bulk endpoints for data in and data out
    endpoint_out = 0x01  # Replace with the correct endpoint address for data out
    endpoint_in = 0x81   # Replace with the correct endpoint address for data in

    data_to_send = bytes([0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, b, r, g, y])
    device.write(endpoint_out, data_to_send)
    
    # Clean up
    device.reset()

def neopixels_face():
    usb_led(1, 1, 1, 1)
def neopixels_hearing():
    usb_led(1, 0, 1, 0)
def neopixels_off():
    usb_led(0, 0, 0, 0)
    

def usb_read_temperature():
    BUFFER_SIZE  = 64
        
    # Specify the path to the libusb-1.0.dll
    usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-x64/dll/libusb-1.0.dll"))
    # usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-Win32/dll/libusb-1.0.dll"))

    # Find the USB device based on its vendor and product IDs
    vendor_id = 0xCAFE# Replace with your device's vendor ID
    product_id = 0x0300  # Replace with your device's product ID

    # Find the device
    device = usb.core.find(idVendor=vendor_id, idProduct=product_id)
    if device is None:
        raise ValueError('Device not found')
#     else:
#     #     print(device)
#         print('found device')

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    device.set_configuration()

    # Find the bulk endpoints for data in and data out
    endpoint_out = 0x01  # Replace with the correct endpoint address for data out
    endpoint_in = 0x81   # Replace with the correct endpoint address for data in

    # -----------------------------
    # Send command 0x52 (read temperature)
    # -----------------------------
    data_to_send = bytearray(64)
    data_to_send[0] = 0x52
    
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)

    # Convert array → bytes
    raw_bytes = bytes(data_received)

    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()

    # -----------------------------
    # Print results
    # -----------------------------
#     print("--------------------")
#     print("first times result :")
#     print("Raw received (hex):", binascii.hexlify(raw_bytes).decode())
#     print("Received string:", received_str)

    # second time
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)

    # Convert array → bytes
    raw_bytes = bytes(data_received)

    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()

    # -----------------------------
    # Print results
    # -----------------------------
#     print("--------------------")
#     print("second times result :")
#     print("Raw received (hex):", binascii.hexlify(raw_bytes).decode())
#     print("Received string:", received_str)


    # third time
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)

    # Convert array → bytes
    raw_bytes = bytes(data_received)

    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()

    # -----------------------------
    # Print results
    # -----------------------------
#     print("--------------------")
#     print("third times result :")
#     print("Raw received (hex):", binascii.hexlify(raw_bytes).decode())
#     print("Received string:", received_str)
    
#     print("--------------------")

    
    # Clean up
    device.reset()
    
    return received_str



def usb_read_humidity():
    BUFFER_SIZE  = 64
        
    # Specify the path to the libusb-1.0.dll
    usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-x64/dll/libusb-1.0.dll"))
    # usb.core.find(backend=usb.backend.libusb1.get_backend(find_library=lambda x: "D:/hong/various_scripts/python/usb/libusb-1.0.26-binaries/libusb-1.0.26-binaries/VS2015-Win32/dll/libusb-1.0.dll"))

    # Find the USB device based on its vendor and product IDs
    vendor_id = 0xCAFE# Replace with your device's vendor ID
    product_id = 0x0300  # Replace with your device's product ID

    # Find the device
    device = usb.core.find(idVendor=vendor_id, idProduct=product_id)
    if device is None:
        raise ValueError('Device not found')
#     else:
#     #     print(device)
#         print('found device')

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    device.set_configuration()

    # Find the bulk endpoints for data in and data out
    endpoint_out = 0x01  # Replace with the correct endpoint address for data out
    endpoint_in = 0x81   # Replace with the correct endpoint address for data in

    # -----------------------------
    # Send command 0x55 (read humidity)
    # -----------------------------
    data_to_send = bytearray(64)
    data_to_send[0] = 0x55
    
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)
    # Convert array → bytes
    raw_bytes = bytes(data_received)
    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()

    # second time
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)
    # Convert array → bytes
    raw_bytes = bytes(data_received)
    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()

    # third time
    device.write(endpoint_out, data_to_send)
    data_received = device.read(endpoint_in, BUFFER_SIZE)
    # Convert array → bytes
    raw_bytes = bytes(data_received)
    # Decode received string
    received_str = raw_bytes.decode("ascii", errors="ignore").strip("\x00").strip()
    
    # Clean up
    device.reset()
    
    return received_str

    
if __name__ == "__main__":
    
    temp = usb_read_temperature()
    if temp is not None:
        print("Final temperature value:", temp)
    else:
        print("No temperature data received")
    
    humidity = usb_read_humidity()
    if humidity is not None:
        print("Final humidity value:", humidity)
    else:
        print("No humidity data received")

    for _ in range(1):
        usb_led(1, 0, 0, 0)
        time.sleep(0.5)
        usb_led(0, 1, 0, 0)
        time.sleep(0.5)
        usb_led(0, 0, 1, 0)
        time.sleep(0.5)
        usb_led(0, 0, 0, 1)
        time.sleep(0.5)
    usb_led(0, 0, 0, 0)
    
    time.sleep(1)
    neopixels_face()
    time.sleep(1)
    neopixels_hearing()
    time.sleep(1)
    neopixels_off()