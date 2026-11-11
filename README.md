# ESP-01S 网页控制继电器（NAS 中转版）

用一块 ESP-01S 控制服务器/主机电源：ESP 自己连 WiFi，定时轮询 NAS 上的一个小 Flask 服务；网页端在设备列表里点「开机 / 强制关机 / OTA 升级」，指令下发到对应板子，板子拉继电器点动。

> 公开版说明：源码里的 WiFi 名称与密码已替换成 `******`，使用前请填成自己的。

## 架构

```
手机/电脑浏览器 ──> http://<NAS_IP>:8090  （Flask 服务 + 网页）
                        ▲
                        │ ESP 每 2 秒 GET /api/poll/<chipId>
                        │
              ESP-01S（多块板子烧同一份固件）
                        │
                   继电器 → 目标主机电源键
```

- 每块板子上电后用芯片 ID（`esp-xxxxx`）自动注册，网页里可以改名（比如「主服务器」）。
- 指令：`kj00` = 点动 2 秒（开机键），`gj00` = 点动 10 秒（强制关机），`ota` = 按 URL 远程升级。

## 目录结构

```
esp01s_relay/         主固件：轮询 NAS + 继电器点动 + OTA（正式版，常运行）
esp_test/             最小 WiFi 连通测试（只连网打印 IP）
diag_relay/           继电器引脚/触发电平诊断（不初始化串口，避免占用 IO1）
nas_server.py         NAS 端 Flask 服务：设备注册、指令队列、改名、OTA 固件分发
index.html            网页界面（纯前端，后端只提供 JSON 接口）
编译固件.ps1           Windows 下用 arduino-cli 一键编译
esp01s_relay.ino.bin  已编译好的固件（OTA 服务的默认固件）
```

## 硬件要点

- 开发板：Generic ESP8266 Module，Flash 1MB，DOUT，40MHz，分区 `1M (FS:64KB OTA:~470KB)`
- 继电器：**高电平触发**，GPIO0 空闲 LOW、吸合 HIGH，点动结束回 LOW
- 烧录：3V3/TXD/RXD/GND，IO0 接 GND 进下载模式，烧完拔掉 IO0 再上电
- 若要做深度睡眠省电版，GPIO16(D0) 必须接 RST 才能定时唤醒

## 固件编译

1. 改 `esp01s_relay/esp01s_relay.ino` 顶部配置区：

```cpp
const char* WIFI_SSID = "******";   // 你的 WiFi 名称
const char* WIFI_PASS = "******";   // 你的 WiFi 密码
const char* NAS_HOST  = "192.168.0.x";
const uint16_t NAS_PORT = 8090;
```

2. 运行 `编译固件.ps1`（需要 arduino-cli 与 esp8266 核心，路径按自己的改）
3. 产物 `esp01s_relay.ino.bin` 既可以串口烧，也可以放到 NAS 供 OTA 使用

## NAS 端部署

```bash
# 目录建议 /opt/relay/
python3 -m pip install flask
# 把 nas_server.py / index.html / esp01s_relay.ino.bin 放同一目录
python3 nas_server.py          # 监听 0.0.0.0:8090
```

systemd 自启示例：

```ini
[Unit]
Description=ESP relay control service
After=network.target

[Service]
WorkingDirectory=/opt/relay
ExecStart=/usr/bin/python3 /opt/relay/nas_server.py
Restart=always

[Install]
WantedBy=multi-user.target
```

设备名与最近指令存在同目录 `devices.json`，重启不丢。

## 接口

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/poll/<chipId>` | ESP 心跳并取走一条待执行指令（新 ID 自动注册） |
| POST | `/api/cmd/<chipId>` | 下发指令，body `{"cmd":"kj00"}` 或 `{"cmd":"ota","url":"..."}` |
| POST | `/api/rename/<chipId>` | 改名，body `{"name":"主服务器"}` |
| POST | `/api/delete/<chipId>` | 删除设备记录 |
| GET | `/api/status` | 设备列表与在线状态 |
| GET | `/firmware.bin` | OTA 固件下载 |

## 已知注意点

- OTA 只有烧过「支持 OTA 的固件」的板子才能远程升级；新板必须先串口烧一次。
- 轮询间隔 `POLL_INTERVAL = 2000`，指令延迟约 2 秒；调小响应更快但更耗电。
- 服务本身没有鉴权，只适合内网使用；要暴露到公网请自行加认证。
