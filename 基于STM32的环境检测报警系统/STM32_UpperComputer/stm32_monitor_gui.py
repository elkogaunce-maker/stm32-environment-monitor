# -*- coding: utf-8 -*-
"""
STM32 environment monitor upper-computer tool.

Protocol:
  PC  -> STM32: @STATUS?\r\n
  PC  -> STM32: @TH=2400\r\n
  STM32 -> PC:  $STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0\r\n
"""

from __future__ import annotations

import ctypes
import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk

try:
    import serial  # type: ignore
    from serial.tools import list_ports  # type: ignore
except Exception:
    serial = None
    list_ports = None


def list_com_ports() -> list[str]:
    if list_ports is not None:
        return [port.device for port in list_ports.comports()]

    try:
        import winreg

        ports: list[str] = []
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DEVICEMAP\SERIALCOMM")
        index = 0
        while True:
            try:
                _, value, _ = winreg.EnumValue(key, index)
                ports.append(str(value))
                index += 1
            except OSError:
                break
        return sorted(set(ports))
    except Exception:
        return []


class PySerialPort:
    def __init__(self, port: str, baudrate: int):
        if serial is None:
            raise RuntimeError("pyserial is not installed")
        self.ser = serial.Serial(port=port, baudrate=baudrate, bytesize=8, parity="N", stopbits=1, timeout=0.1)

    def write(self, data: bytes) -> None:
        self.ser.write(data)

    def read(self, size: int = 256) -> bytes:
        return self.ser.read(size)

    def close(self) -> None:
        self.ser.close()


class DCB(ctypes.Structure):
    _fields_ = [
        ("DCBlength", ctypes.c_uint32),
        ("BaudRate", ctypes.c_uint32),
        ("flags", ctypes.c_uint32),
        ("wReserved", ctypes.c_uint16),
        ("XonLim", ctypes.c_uint16),
        ("XoffLim", ctypes.c_uint16),
        ("ByteSize", ctypes.c_uint8),
        ("Parity", ctypes.c_uint8),
        ("StopBits", ctypes.c_uint8),
        ("XonChar", ctypes.c_char),
        ("XoffChar", ctypes.c_char),
        ("ErrorChar", ctypes.c_char),
        ("EofChar", ctypes.c_char),
        ("EvtChar", ctypes.c_char),
        ("wReserved1", ctypes.c_uint16),
    ]


class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ("ReadIntervalTimeout", ctypes.c_uint32),
        ("ReadTotalTimeoutMultiplier", ctypes.c_uint32),
        ("ReadTotalTimeoutConstant", ctypes.c_uint32),
        ("WriteTotalTimeoutMultiplier", ctypes.c_uint32),
        ("WriteTotalTimeoutConstant", ctypes.c_uint32),
    ]


class WinSerialPort:
    GENERIC_READ = 0x80000000
    GENERIC_WRITE = 0x40000000
    OPEN_EXISTING = 3
    INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value
    PURGE_RXCLEAR = 0x0008
    PURGE_TXCLEAR = 0x0004

    def __init__(self, port: str, baudrate: int):
        self.kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        path = port if port.startswith(r"\\.") else rf"\\.\{port}"
        self.handle = self.kernel32.CreateFileW(
            path,
            self.GENERIC_READ | self.GENERIC_WRITE,
            0,
            None,
            self.OPEN_EXISTING,
            0,
            None,
        )
        if self.handle == self.INVALID_HANDLE_VALUE:
            raise OSError(ctypes.get_last_error(), f"Cannot open {port}")

        self.kernel32.SetupComm(self.handle, 4096, 4096)
        self._configure(baudrate)
        self.kernel32.PurgeComm(self.handle, self.PURGE_RXCLEAR | self.PURGE_TXCLEAR)

    def _configure(self, baudrate: int) -> None:
        dcb = DCB()
        dcb.DCBlength = ctypes.sizeof(DCB)
        if not self.kernel32.GetCommState(self.handle, ctypes.byref(dcb)):
            raise OSError(ctypes.get_last_error(), "GetCommState failed")

        dcb.BaudRate = baudrate
        dcb.ByteSize = 8
        dcb.Parity = 0
        dcb.StopBits = 0
        dcb.flags = 0x00000001

        if not self.kernel32.SetCommState(self.handle, ctypes.byref(dcb)):
            raise OSError(ctypes.get_last_error(), "SetCommState failed")

        timeouts = COMMTIMEOUTS()
        timeouts.ReadIntervalTimeout = 50
        timeouts.ReadTotalTimeoutMultiplier = 0
        timeouts.ReadTotalTimeoutConstant = 50
        timeouts.WriteTotalTimeoutMultiplier = 0
        timeouts.WriteTotalTimeoutConstant = 100
        if not self.kernel32.SetCommTimeouts(self.handle, ctypes.byref(timeouts)):
            raise OSError(ctypes.get_last_error(), "SetCommTimeouts failed")

    def write(self, data: bytes) -> None:
        written = ctypes.c_uint32(0)
        buf = ctypes.create_string_buffer(data)
        ok = self.kernel32.WriteFile(self.handle, buf, len(data), ctypes.byref(written), None)
        if not ok:
            raise OSError(ctypes.get_last_error(), "WriteFile failed")

    def read(self, size: int = 256) -> bytes:
        buf = ctypes.create_string_buffer(size)
        read_count = ctypes.c_uint32(0)
        ok = self.kernel32.ReadFile(self.handle, buf, size, ctypes.byref(read_count), None)
        if not ok:
            raise OSError(ctypes.get_last_error(), "ReadFile failed")
        return bytes(buf.raw[: read_count.value])

    def close(self) -> None:
        if getattr(self, "handle", None):
            self.kernel32.CloseHandle(self.handle)
            self.handle = None


def open_serial_port(port: str, baudrate: int):
    if serial is not None:
        return PySerialPort(port, baudrate)
    return WinSerialPort(port, baudrate)


class MonitorApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("STM32 环境监测上位机")
        self.geometry("860x560")
        self.minsize(820, 520)

        self.serial_port = None
        self.reader_thread: threading.Thread | None = None
        self.stop_event = threading.Event()
        self.rx_queue: queue.Queue[str] = queue.Queue()
        self.rx_buffer = ""

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="9600")
        self.threshold_var = tk.StringVar(value="2400")
        self.motor_var = tk.StringVar(value="80")
        self.raw_cmd_var = tk.StringVar(value="@STATUS?")
        self.auto_query_var = tk.BooleanVar(value=False)

        self.adc_var = tk.StringVar(value="--")
        self.mv_var = tk.StringVar(value="--")
        self.th_var = tk.StringVar(value="--")
        self.alarm_var = tk.StringVar(value="--")
        self.mode_var = tk.StringVar(value="--")
        self.version_var = tk.StringVar(value="--")
        self.conn_var = tk.StringVar(value="未连接")

        self._build_ui()
        self.refresh_ports()
        self.after(80, self._process_rx_queue)
        self.after(1000, self._auto_query_tick)

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=12)
        root.pack(fill=tk.BOTH, expand=True)

        conn = ttk.LabelFrame(root, text="连接")
        conn.pack(fill=tk.X)
        ttk.Label(conn, text="串口").grid(row=0, column=0, padx=6, pady=8)
        self.port_combo = ttk.Combobox(conn, textvariable=self.port_var, width=12, state="readonly")
        self.port_combo.grid(row=0, column=1, padx=6, pady=8)
        ttk.Button(conn, text="刷新", command=self.refresh_ports).grid(row=0, column=2, padx=6, pady=8)
        ttk.Label(conn, text="波特率").grid(row=0, column=3, padx=6, pady=8)
        ttk.Combobox(conn, textvariable=self.baud_var, values=["9600", "115200"], width=10, state="readonly").grid(
            row=0, column=4, padx=6, pady=8
        )
        self.connect_button = ttk.Button(conn, text="连接", command=self.toggle_connect)
        self.connect_button.grid(row=0, column=5, padx=6, pady=8)
        ttk.Label(conn, textvariable=self.conn_var).grid(row=0, column=6, padx=12, pady=8, sticky=tk.W)
        conn.columnconfigure(7, weight=1)

        status = ttk.LabelFrame(root, text="实时状态")
        status.pack(fill=tk.X, pady=10)
        fields = [
            ("ADC", self.adc_var),
            ("电压 mV", self.mv_var),
            ("阈值 mV", self.th_var),
            ("报警", self.alarm_var),
            ("模式", self.mode_var),
            ("版本", self.version_var),
        ]
        for i, (label, var) in enumerate(fields):
            box = ttk.Frame(status)
            box.grid(row=0, column=i, padx=10, pady=10, sticky=tk.NSEW)
            ttk.Label(box, text=label).pack(anchor=tk.W)
            ttk.Label(box, textvariable=var, font=("Microsoft YaHei UI", 15, "bold")).pack(anchor=tk.W)
            status.columnconfigure(i, weight=1)

        cmds = ttk.LabelFrame(root, text="命令")
        cmds.pack(fill=tk.X)
        ttk.Button(cmds, text="查询状态", command=lambda: self.send_command("@STATUS?")).grid(row=0, column=0, padx=6, pady=8)
        ttk.Button(cmds, text="查询版本", command=lambda: self.send_command("@VERSION?")).grid(row=0, column=1, padx=6, pady=8)
        ttk.Label(cmds, text="阈值 mV").grid(row=0, column=2, padx=6, pady=8)
        ttk.Entry(cmds, textvariable=self.threshold_var, width=8).grid(row=0, column=3, padx=6, pady=8)
        ttk.Button(cmds, text="设置阈值", command=self.set_threshold).grid(row=0, column=4, padx=6, pady=8)
        ttk.Button(cmds, text="报警开", command=lambda: self.send_command("@ALARM=ON")).grid(row=1, column=0, padx=6, pady=8)
        ttk.Button(cmds, text="报警关", command=lambda: self.send_command("@ALARM=OFF")).grid(row=1, column=1, padx=6, pady=8)
        ttk.Button(cmds, text="自动模式", command=lambda: self.send_command("@ALARM=AUTO")).grid(row=1, column=2, padx=6, pady=8)
        ttk.Label(cmds, text="电机速度").grid(row=1, column=3, padx=6, pady=8)
        ttk.Entry(cmds, textvariable=self.motor_var, width=8).grid(row=1, column=4, padx=6, pady=8)
        ttk.Button(cmds, text="设置电机", command=self.set_motor).grid(row=1, column=5, padx=6, pady=8)
        ttk.Checkbutton(cmds, text="每秒自动查询", variable=self.auto_query_var).grid(row=1, column=6, padx=12, pady=8)
        cmds.columnconfigure(7, weight=1)

        manual = ttk.LabelFrame(root, text="手动命令")
        manual.pack(fill=tk.X, pady=10)
        ttk.Entry(manual, textvariable=self.raw_cmd_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=6, pady=8)
        ttk.Button(manual, text="发送", command=lambda: self.send_command(self.raw_cmd_var.get())).pack(side=tk.LEFT, padx=6)

        log_frame = ttk.LabelFrame(root, text="通信日志")
        log_frame.pack(fill=tk.BOTH, expand=True)
        self.log_text = tk.Text(log_frame, height=12, wrap=tk.WORD)
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar = ttk.Scrollbar(log_frame, command=self.log_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.configure(yscrollcommand=scrollbar.set)

    def refresh_ports(self) -> None:
        ports = list_com_ports()
        self.port_combo["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])
        self.log(f"发现串口: {', '.join(ports) if ports else '无'}")

    def toggle_connect(self) -> None:
        if self.serial_port is None:
            self.connect()
        else:
            self.disconnect()

    def connect(self) -> None:
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("提示", "请先选择串口")
            return
        try:
            baudrate = int(self.baud_var.get())
            self.serial_port = open_serial_port(port, baudrate)
        except Exception as exc:
            self.serial_port = None
            messagebox.showerror("连接失败", str(exc))
            return

        self.stop_event.clear()
        self.reader_thread = threading.Thread(target=self._reader_loop, daemon=True)
        self.reader_thread.start()
        self.conn_var.set(f"已连接 {port} @ {self.baud_var.get()}")
        self.connect_button.configure(text="断开")
        self.log(f"已连接 {port}")

    def disconnect(self) -> None:
        self.stop_event.set()
        if self.serial_port is not None:
            try:
                self.serial_port.close()
            except Exception:
                pass
        self.serial_port = None
        self.conn_var.set("未连接")
        self.connect_button.configure(text="连接")
        self.log("已断开")

    def _reader_loop(self) -> None:
        while not self.stop_event.is_set():
            try:
                if self.serial_port is None:
                    break
                data = self.serial_port.read(256)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    self.rx_queue.put(text)
                else:
                    time.sleep(0.02)
            except Exception as exc:
                self.rx_queue.put(f"\n$ERR=READ_FAIL:{exc}\n")
                break

    def _process_rx_queue(self) -> None:
        while True:
            try:
                text = self.rx_queue.get_nowait()
            except queue.Empty:
                break
            self.rx_buffer += text
            while "\n" in self.rx_buffer:
                line, self.rx_buffer = self.rx_buffer.split("\n", 1)
                line = line.strip()
                if line:
                    self.log(f"RX: {line}")
                    self.parse_response(line)
        self.after(80, self._process_rx_queue)

    def _auto_query_tick(self) -> None:
        if self.auto_query_var.get() and self.serial_port is not None:
            self.send_command("@STATUS?")
        self.after(1000, self._auto_query_tick)

    def send_command(self, cmd: str) -> None:
        if self.serial_port is None:
            messagebox.showwarning("提示", "请先连接串口")
            return
        cmd = cmd.strip()
        if not cmd:
            return
        if not cmd.startswith("@"):
            cmd = "@" + cmd
        payload = (cmd + "\r\n").encode("ascii", errors="ignore")
        try:
            self.serial_port.write(payload)
            self.log(f"TX: {cmd}")
        except Exception as exc:
            messagebox.showerror("发送失败", str(exc))

    def set_threshold(self) -> None:
        value = self.threshold_var.get().strip()
        if not value.isdigit():
            messagebox.showwarning("提示", "阈值请输入数字，例如 2400")
            return
        mv = int(value)
        if not 0 <= mv <= 3300:
            messagebox.showwarning("提示", "阈值范围建议为 0-3300 mV")
            return
        self.send_command(f"@TH={mv}")

    def set_motor(self) -> None:
        value = self.motor_var.get().strip()
        if not value.lstrip("-").isdigit():
            messagebox.showwarning("提示", "电机速度请输入数字，例如 80")
            return
        speed = int(value)
        if not -100 <= speed <= 100:
            messagebox.showwarning("提示", "电机速度范围建议为 -100 到 100")
            return
        self.send_command(f"@MOTOR={speed}")

    def parse_response(self, line: str) -> None:
        if line.startswith("$STATUS"):
            self._parse_status(line)
        elif line.startswith("$VERSION="):
            self.version_var.set(line.split("=", 1)[1])
        elif line.startswith("$OK"):
            self.log("解析: 命令执行成功")
        elif line.startswith("$ERR"):
            self.log(f"解析: 设备返回错误 {line}")

    def _parse_status(self, line: str) -> None:
        fields: dict[str, str] = {}
        for part in line.split(",")[1:]:
            if "=" in part:
                key, value = part.split("=", 1)
                fields[key.strip().upper()] = value.strip()

        if "ADC" in fields:
            self.adc_var.set(fields["ADC"])
        if "MV" in fields:
            self.mv_var.set(fields["MV"])
        if "TH" in fields:
            self.th_var.set(fields["TH"])
        if "ALARM" in fields:
            self.alarm_var.set("报警" if fields["ALARM"] in ("1", "ON") else "正常")
        if "MODE" in fields:
            mode_map = {
                "AUTO": "自动",
                "MANUAL_ON": "手动开",
                "MANUAL_OFF": "手动关",
            }
            self.mode_var.set(mode_map.get(fields["MODE"], fields["MODE"]))

    def log(self, msg: str) -> None:
        now = time.strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{now}] {msg}\n")
        self.log_text.see(tk.END)

    def destroy(self) -> None:
        self.disconnect()
        super().destroy()


if __name__ == "__main__":
    app = MonitorApp()
    app.mainloop()
