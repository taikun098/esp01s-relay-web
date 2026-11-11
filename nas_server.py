# -*- coding: utf-8 -*-
"""
ESP01S 继电器集中控制服务（自动注册版）
- ESP 上电自动用 chip_id 注册，网页里可改名
- 数据持久化到 devices.json，重启不丢
"""
import json, os, time, threading
from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__, static_folder=None)
DB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "devices.json")

_dev_lock = threading.Lock()
_devices = {}   # {chip_id: {"name":..., "last_seen":..., "pending_cmd":..., "last_cmd":..., "last_cmd_time":...}}

def load_db():
    global _devices
    if os.path.exists(DB):
        try:
            saved = json.load(open(DB, encoding="utf-8"))
            for cid, info in saved.items():
                _devices[cid] = {
                    "name": info.get("name", cid),
                    "last_seen": 0, "pending_cmd": None,
                    "last_cmd": None, "last_cmd_time": 0,
                }
        except Exception as e:
            print("load db err:", e)

def save_db():
    out = {cid: {"name": v["name"]} for cid, v in _devices.items()}
    json.dump(out, open(DB, "w", encoding="utf-8"), ensure_ascii=False, indent=2)

load_db()

@app.route("/firmware.bin")
def firmware():
    return send_from_directory(os.path.dirname(os.path.abspath(__file__)), "esp01s_relay.ino.bin")

@app.route("/")
def index():
    return send_from_directory(os.path.dirname(os.path.abspath(__file__)), "index.html")

@app.route("/api/poll/<cid>")
def poll(cid):
    """ESP 心跳+拉指令。新 chip_id 自动注册。"""
    with _dev_lock:
        if cid not in _devices:
            _devices[cid] = {"name": cid, "last_seen":0, "pending_cmd":None,
                             "last_cmd":None, "last_cmd_time":0}
            save_db()
        _devices[cid]["last_seen"] = time.time()
        cmd = _devices[cid]["pending_cmd"]
        _devices[cid]["pending_cmd"] = None
        if cmd:
            _devices[cid]["last_cmd"] = cmd.get("cmd") if isinstance(cmd, dict) else cmd
            _devices[cid]["last_cmd_time"] = time.time()
    if isinstance(cmd, dict):
        return jsonify({"cmd": cmd.get("cmd"), "url": cmd.get("url")})
    return jsonify({"cmd": cmd})

@app.route("/api/cmd/<cid>", methods=["POST"])
def send_cmd(cid):
    data = request.get_json(silent=True) or {}
    cmd = data.get("cmd", "").strip()
    if cmd not in ("kj00", "gj00", "ota"):
        return jsonify({"error": "bad cmd"}), 400
    if cmd == "ota":
        url = (data.get("url") or "").strip() or "http://192.168.0.x:8090/firmware.bin"
        payload = {"cmd": "ota", "url": url}
    else:
        payload = cmd
    with _dev_lock:
        if cid not in _devices:
            return jsonify({"error": "device not found"}), 404
        _devices[cid]["pending_cmd"] = payload
    return jsonify({"ok": True})

@app.route("/api/rename/<cid>", methods=["POST"])
def rename(cid):
    data = request.get_json(silent=True) or {}
    name = (data.get("name") or "").strip()[:20]
    if not name:
        return jsonify({"error": "empty name"}), 400
    with _dev_lock:
        if cid not in _devices:
            return jsonify({"error": "device not found"}), 404
        _devices[cid]["name"] = name
        save_db()
    return jsonify({"ok": True})

@app.route("/api/delete/<cid>", methods=["POST"])
def delete(cid):
    with _dev_lock:
        _devices.pop(cid, None)
        save_db()
    return jsonify({"ok": True})

@app.route("/api/status")
def status():
    now = time.time()
    out = []
    with _dev_lock:
        for cid, s in _devices.items():
            age = now - s["last_seen"] if s["last_seen"] else 999999
            out.append({
                "id": cid, "name": s["name"],
                "online": age < 15,
                "last_seen_ago": int(age) if s["last_seen"] else None,
                "last_cmd": s["last_cmd"],
                "last_cmd_time": s["last_cmd_time"],
            })
    out.sort(key=lambda x: (not x["online"], x["name"]))
    return jsonify({"devices": out})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8090, debug=False)
