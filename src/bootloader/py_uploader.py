import argparse
import sys
import time

import serial

CMD_ERASE = 0x01
CMD_WRITE = 0x02
CMD_READ = 0x03
CMD_JUMP = 0x04
CMD_INFO = 0x05

ACK_OK = b"O"
ACK_ERROR = b"E"
ACK_INFO = b"I"
ACK_UNKNOWN = b"?"
DBG_MARKER = b"D"
BLOCK_SIZE = 32

DEBUG_NAMES = {
    0x10: "WRITE header received, length",
    0x11: "WRITE block offset",
    0x12: "Command byte received",
    0x20: "FLASH write address",
    0x21: "FLASH PG set, CR2",
    0x22: "FLASH store loop done, SR2",
    0x23: "FLASH write done, address",
    0xE0: "FLASH address/range error",
    0xE1: "FLASH wait timeout, SR2",
    0xE2: "FLASH verify error address",
    0xE3: "FLASH error flags, SR2",
    0xF0: "HardFault, CFSR",
    0xF1: "MemManage fault, CFSR",
    0xF2: "BusFault, CFSR",
}


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
    while True:
        response = read_response_byte(ser, stage)
        if response == ACK_OK:
            return
        if response == ACK_ERROR:
            raise RuntimeError(f"{stage}: bootloader returned error")
        if not response:
            raise RuntimeError(f"{stage}: timeout waiting for ACK")
        raise RuntimeError(f"{stage}: unexpected response {response!r}")


def read_response_byte(ser, stage):
    while True:
        response = ser.read(1)
        print(f"    [DBG] {response}")
        if response != DBG_MARKER:
            return response

        payload = ser.read(5)
        if len(payload) != 5:
            raise RuntimeError(f"{stage}: incomplete debug packet {payload!r}")

        code = payload[0]
        value = int.from_bytes(payload[1:5], "little")
        name = DEBUG_NAMES.get(code, f"debug 0x{code:02X}")
        print(f"    [DBG] {name}: 0x{value:08X} ({value})")


def erase(ser):
    print("[+] Erase application area")
    ser.write(bytes([CMD_ERASE]))
    wait_ack(ser, "erase")


def read_info(ser):
    print("[+] Read bootloader info")
    ser.write(bytes([CMD_INFO]))
    response = read_response_byte(ser, "info")
    if response == "?":
        raise RuntimeError("info: bootloader does not support INFO command; old bootloader is probably running")
    if response != 'I':
        raise RuntimeError(f"info: unexpected response {response}")
    print(f"info: unexpected response {response}")

    payload = ser.read(16)
    if len(payload) != 16:
        raise RuntimeError(f"info: expected 16 bytes, got {len(payload)}")

    version = int.from_bytes(payload[0:4], "little")
    app_base = int.from_bytes(payload[4:8], "little")
    app_end = int.from_bytes(payload[8:12], "little")
    sr2 = int.from_bytes(payload[12:16], "little")

    print(f"[+] Bootloader version: 0x{version:08X}")
    print(f"[+] Application area:  0x{app_base:08X}..0x{app_end - 1:08X}")
    print(f"[+] FLASH SR2:         0x{sr2:08X}")

    if version != 0x00040000:
        raise RuntimeError("info: bootloader version is not 0x00040000; please rebuild and upload the latest bootloader")

    if app_base != 0x08100000 or app_end != 0x08200000:
        raise RuntimeError("info: bootloader is not configured for Flash Bank 2 application area")


def write_firmware(ser, firmware):
    print(f"[+] Start write (command)")
    ser.write(bytes([CMD_WRITE]))
    wait_ack(ser, "write command")

    print(f"[+] Start write (header)")
    ser.write(u32le(0) + u32le(len(firmware)))
    wait_ack(ser, "write header")

    print(f"[+] Start write (app) {len(firmware)} bytes")
    for offset in range(0, len(firmware), BLOCK_SIZE):
        ser.write(firmware[offset : offset + BLOCK_SIZE])
        wait_ack(ser, f"write block at offset {offset}")

    print(f"[+] End write {len(firmware)} bytes")


# def read_firmware(ser, size):
#     print(f"[+] Start read back {size} bytes")
#     ser.write(bytes([CMD_READ]) + u32le(0) + u32le(size))
#     wait_ack(ser, "read")

#     data = ser.read(size)
#     if len(data) != size:
#         raise RuntimeError(f"read: expected {size} bytes, got {len(data)}")
#     return data


def read_firmware(ser, size):
    print(f"[+] Read back {size} bytes")
    ser.write(bytes([CMD_READ]))
    wait_ack(ser, "read command")

    ser.write(u32le(0) + u32le(size))
    wait_ack(ser, "read header")

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
    parser.add_argument("--timeout", type=float, default=30.0, help="UART timeout in seconds")
    parser.add_argument("--skip-info", action="store_true", help="Skip bootloader INFO check")
    parser.add_argument("--no-verify", action="store_true", help="Skip read-back verification")
    parser.add_argument("--no-jump", action="store_true", help="Do not start application after upload")
    args = parser.parse_args()

    with open(args.firmware, "rb") as file:
        firmware = file.read()

    if not firmware:
        raise RuntimeError("firmware file is empty")

    print(f"[+] Firmware: {args.firmware} ({len(firmware)} bytes)")
    print(f"[+] Port: {args.port}, baud: {args.baud}, timeout: {args.timeout}s")

    with serial.Serial(args.port, args.baud, timeout=args.timeout, write_timeout=args.timeout) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()

        if not args.skip_info:
            read_info(ser)

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
