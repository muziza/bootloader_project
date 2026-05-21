import argparse
import sys
import time

import serial

CMD_ERASE = 0x01
CMD_WRITE = 0x02
CMD_READ = 0x03
CMD_JUMP = 0x04

ACK_OK = b"O"
ACK_ERROR = b"E"
BLOCK_SIZE = 32


def u32le(value):
    return bytes(
        (
            value & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF,
        )
    )


def wait_ack(ser, stage):
    response = ser.read(1)
    if response == ACK_OK:
        return
    if response == ACK_ERROR:
        raise RuntimeError(f"{stage}: bootloader returned error")
    if not response:
        raise RuntimeError(f"{stage}: timeout waiting for ACK")
    raise RuntimeError(f"{stage}: unexpected response {response!r}")


def erase(ser):
    print("[+] Erase application area")
    ser.write(bytes([CMD_ERASE]))
    wait_ack(ser, "erase")


def write_firmware(ser, firmware):
    print(f"[+] Write {len(firmware)} bytes")
    ser.write(bytes([CMD_WRITE]) + u32le(0) + u32le(len(firmware)))

    for offset in range(0, len(firmware), BLOCK_SIZE):
        chunk = firmware[offset : offset + BLOCK_SIZE]
        if len(chunk) < BLOCK_SIZE:
            chunk += b"\xFF" * (BLOCK_SIZE - len(chunk))
        ser.write(chunk)

    wait_ack(ser, "write")


def read_firmware(ser, size):
    print(f"[+] Read back {size} bytes")
    ser.write(bytes([CMD_READ]) + u32le(0) + u32le(size))
    wait_ack(ser, "read")

    data = ser.read(size)
    if len(data) != size:
        raise RuntimeError(f"read: expected {size} bytes, got {len(data)}")
    return data


def jump(ser):
    print("[+] Jump to application")
    ser.write(bytes([CMD_JUMP]))
    wait_ack(ser, "jump")


def main():
    parser = argparse.ArgumentParser(description="STM32H745 UART bootloader uploader")
    parser.add_argument("firmware", help="Path to application .bin file")
    parser.add_argument("--port", default="COM5", help="UART port, default: COM5")
    parser.add_argument("--baud", type=int, default=115200, help="UART baudrate")
    parser.add_argument("--no-verify", action="store_true", help="Skip read-back verification")
    parser.add_argument("--no-jump", action="store_true", help="Do not start application after upload")
    args = parser.parse_args()

    with open(args.firmware, "rb") as file:
        firmware = file.read()

    if not firmware:
        raise RuntimeError("firmware file is empty")

    print(f"[+] Firmware: {args.firmware} ({len(firmware)} bytes)")
    print(f"[+] Port: {args.port}, baud: {args.baud}")

    with serial.Serial(args.port, args.baud, timeout=3, write_timeout=3) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()

        erase(ser)
        write_firmware(ser, firmware)

        if not args.no_verify:
            dumped = read_firmware(ser, len(firmware))
            if dumped != firmware:
                raise RuntimeError("verify: read-back data differs from firmware")
            print("[+] Verify OK")

        if not args.no_jump:
            jump(ser)

    print("[+] Done")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"[-] {exc}", file=sys.stderr)
        sys.exit(1)
