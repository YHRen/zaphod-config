#!/usr/bin/env python3
"""Check generated configuration and peripheral ownership, not source intent."""
import argparse
import re
import sys
import struct
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--diagnostic", action="store_true")
    args = parser.parse_args()
    generated = args.build / "zephyr"
    config = dict(re.findall(r"^(CONFIG_\w+)=(.*)$", (generated / ".config").read_text(), re.M))
    dts = (generated / "zephyr.dts").read_text()
    link_map = (generated / "zmk.map").read_text()
    workspace = args.build.resolve().parents[1]
    sys.path.insert(0, str(workspace / "zephyr/scripts/dts/python-devicetree/src"))
    from devicetree import dtlib
    tree = dtlib.DT(str(generated / "zephyr.dts"))

    def node(label):
        return tree.label2node[label]

    def numbers(label, prop, expected):
        actual = cells(node(label), prop)
        if actual != expected:
            raise SystemExit(f"Unexpected {label}.{prop}: {actual} != {expected}")

    def cells(item, prop):
        value = item.props[prop].value
        return list(struct.unpack(">" + "I" * (len(value) // 4), value))

    def gpio(label, prop, port, pin):
        numbers(label, prop, [node(port).props["phandle"].to_num(), pin, 0])

    def status(label, expected):
        value = node(label).props.get("status")
        actual = value.to_string() if value else "okay"
        if actual != expected:
            raise SystemExit(f"Expected {label} {expected}, got {actual}")
    def enabled(*keys):
        for key in keys:
            if config.get("CONFIG_" + key) != "y":
                raise SystemExit(f"Expected {key}=y")
    def disabled(*keys):
        for key in keys:
            if config.get("CONFIG_" + key) == "y":
                raise SystemExit(f"Expected {key} disabled")
    enabled("ZMK_USB", "ZMK_BLE")
    if args.diagnostic:
        enabled("ASSERT", "ZMK_USB_LOGGING", "UART_CONSOLE", "LOG_BACKEND_UART")
    if "trackpoint" in args.target:
        enabled("ZMK_POINTING", "PS2_UART", "ZMK_INPUT_MOUSE_PS2", "PM_DEVICE",
                "UART_INTERRUPT_DRIVEN", "UART_0_INTERRUPT_DRIVEN")
        disabled("ZMK_INPUT_LISTENER_PS2")
        numbers("uart0", "current-speed", [14400])
        numbers("gpiote", "interrupts", [6, 0])
        status("uart0", "okay")
        if "disable-clicking" not in node("mouse_ps2").props:
            raise SystemExit("Movement-only mouse must disable clicking")
        if node("trackpoint_listener").props["device"].to_node() != node("mouse_ps2"):
            raise SystemExit("Unexpected TrackPoint listener source")
        if node("mouse_ps2").props["ps2-device"].to_node() != node("uart_ps2"):
            raise SystemExit("Unexpected TrackPoint transport")
    if args.target == "zaphod-trackpoint-display-replacement":
        enabled("PS2_UART_OPEN_DRAIN")
        disabled("ZMK_DISPLAY", "DISPLAY", "LVGL", "ZAPHOD_BONGO_CAT", "LS0XX", "SPI")
        if "zephyr,display" in dts:
            raise SystemExit("Stale chosen display property")
        for label in ["ls0xx", "spi0", "i2c0"]:
            status(label, "disabled")
        for prop in ["pinctrl-0", "pinctrl-1", "cs-gpios"]:
            if prop in node("spi0").props:
                raise SystemExit(f"Stale SPI ownership: {prop}")
        gpio("uart_ps2", "scl-gpios", "gpio0", 7)
        gpio("uart_ps2", "sda-gpios", "gpio0", 5)
        gpio("mouse_ps2", "rst-gpios", "gpio0", 4)
        gpio("blue_led", "gpios", "gpio1", 9)
        for label, expected in [("uart0_ps2_default", [27, 0x10005]),
                                ("uart0_ps2_off", [27, 0x1001a])]:
            actual = node(label).nodes["group1"].props["psels"].to_nums()
            if actual != expected:
                raise SystemExit(f"Unexpected {label} UART pin selection: {actual}")
        # Keyboard matrix and boot/storage partitions stay fixed.
        for prop, pins in [
            ("col-gpios", [(1,4),(1,6),(0,9),(0,10),(0,22),(0,2),(0,28),(1,15),(0,30),(0,29)]),
            ("row-gpios", [(1,0),(1,3),(1,1),(1,2)]),
        ]:
            flags = 0x20 if prop == "row-gpios" else 0
            expected = [cell for port,pin in pins
                        for cell in [node(f"gpio{port}").props["phandle"].to_num(),pin,flags]]
            numbers("kscan", prop, expected)
        numbers("code_partition", "reg", [0x1000, 0xd3000])
        numbers("storage_partition", "reg", [0xd4000, 0x20000])
        numbers("boot_partition", "reg", [0xf4000, 0xc000])
        numbers("vbatt", "io-channels", [node("adc").props["phandle"].to_num(), 7])
        if "unused_display_pins.c.obj" not in link_map:
            raise SystemExit("Missing unused-display-pin initialization")
        # Inspect every enabled GPIO/pinctrl consumer for conflicts on J1.4-.6.
        reserved = {(0,4), (0,5), (0,7)}
        allowed_gpio = {(node("mouse_ps2").path, "rst-gpios"),
                        (node("uart_ps2").path, "scl-gpios"),
                        (node("uart_ps2").path, "sda-gpios")}
        ports = {node(f"gpio{p}").props["phandle"].to_num(): p for p in [0,1]}

        def active(item):
            while item is not None:
                prop = item.props.get("status")
                if prop and prop.to_string() not in ["okay", "ok"]:
                    return False
                item = item.parent
            return True

        for item in tree.node_iter():
            if not active(item):
                continue
            for name, prop in item.props.items():
                if name == "gpios" or name.endswith("-gpios"):
                    values = cells(item, name)
                    for i in range(0, len(values), 3):
                        if i+2 >= len(values) or values[i] not in ports:
                            raise SystemExit(f"Unexpected GPIO format: {item.path}.{name}")
                        pin = (ports[values[i]], values[i+1])
                        if pin in reserved and (item.path, name) not in allowed_gpio:
                            raise SystemExit(f"Conflicting GPIO owner: {item.path}.{name}")
                if re.fullmatch(r"pinctrl-\d+", name):
                    for state in prop.to_nodes():
                        for group in state.nodes.values():
                            for value in group.props.get("psels").to_nums():
                                pin = ((value & 0x7f) // 32, (value & 0x7f) % 32)
                                if pin in reserved and item != node("uart0"):
                                    raise SystemExit(f"Conflicting pinctrl owner: {item.path}")
        for object_name in ["zaphod_status_screen.c.obj", "zaphod_bongo_cat_widget.c.obj", "zaphod_bongo_cat_images.c.obj"]:
            if object_name in link_map:
                raise SystemExit(f"Unexpected display object: {object_name}")
    elif args.target in ["zaphod", "zaphod-trackpoint"]:
        enabled("ZMK_DISPLAY", "DISPLAY", "LVGL", "ZAPHOD_BONGO_CAT", "LS0XX", "SPI", "LV_USE_THEME_MONO")
        disabled("PS2_UART_OPEN_DRAIN")
        status("spi0", "okay")
        if args.target == "zaphod-trackpoint":
            gpio("uart_ps2", "scl-gpios", "gpio1", 11)
            gpio("uart_ps2", "sda-gpios", "gpio1", 10)
            gpio("mouse_ps2", "rst-gpios", "gpio1", 13)
    print(f"Generated firmware configuration verified: {args.target}")


if __name__ == "__main__":
    main()
