# 一键编译 ESP01S 继电器固件
# 改完 esp01s_relay.ino 里的 WiFi 信息后，双击运行这个脚本
$cli = "C:\tools\arduino-cli\arduino-cli.exe"
$src = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = "C:\tools\build"

Write-Host "=== 编译中 ===" -ForegroundColor Cyan
& $cli compile -b "esp8266:esp8266:generic:eesz=1M64,FlashMode=dout,FlashFreq=40" --build-path $build "$src\esp01s_relay" 2>&1 | Select-Object -Last 8

if($LASTEXITCODE -eq 0){
  $bin = Join-Path $build "esp01s_relay.ino.bin"
  if(Test-Path -LiteralPath $bin){
    $out = "$src\esp01s_relay.ino.bin"
    Copy-Item $bin $out -Force
    Write-Host "`n=== 编译成功 ===" -ForegroundColor Green
    Write-Host "固件: $out"
    Write-Host ("大小: {0:N1} KB" -f ((Get-Item $bin).Length/1KB))
  }
} else {
  Write-Host "`n=== 编译失败，看上面错误 ===" -ForegroundColor Red
}
Read-Host "`n按回车退出"
