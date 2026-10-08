# Hastane Yönetimi - dağıtım klasörünü hazırlar (dist\HospitalManager)
# 1) Release derlemesi  2) windeployqt ile Qt dosyaları  3) MSYS2'den gelen diğer DLL'ler
# Kullanım (proje klasöründe): powershell -ExecutionPolicy Bypass -File installer\deploy.ps1

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$msys = 'C:\msys64\ucrt64\bin'
$build = Join-Path $root 'build-release'
$dist = Join-Path $root 'dist\HospitalManager'

Write-Host '1/3 Release derlemesi...'
cmake -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release | Out-Null
cmake --build $build
if ($LASTEXITCODE -ne 0) { throw 'Derleme başarısız.' }

Write-Host '2/3 Qt dosyaları kopyalanıyor...'
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item (Join-Path $build 'HospitalManager.exe') $dist
& (Join-Path $msys 'windeployqt6.exe') --release --no-translations --no-system-d3d-compiler --no-opengl-sw (Join-Path $dist 'HospitalManager.exe') | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'windeployqt başarısız.' }

# Kullanılmayan eklentiler: sadece SQLite sürücüsü gerekli; glib ağ eklentisi ~10 MB ek kütüphane getirir
Get-ChildItem (Join-Path $dist 'sqldrivers') -Exclude 'qsqlite.dll' | Remove-Item -Force
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $dist 'networkinformation\qglib.dll')

Write-Host '3/3 Diğer bağımlılıklar (MinGW, libsodium, ICU...) kopyalanıyor...'
# Her .exe ve .dll'in ihtiyaç duyduğu DLL'leri objdump ile bulur; MSYS2 klasöründe olanları kopyalar.
# Yeni kopyalanan DLL'lerin de bağımlılıkları olabileceği için hiçbir şey eklenmeyene kadar tekrarlar.
do {
    $added = 0
    $binaries = Get-ChildItem $dist -Recurse -Include *.exe, *.dll
    foreach ($binary in $binaries) {
        $deps = & (Join-Path $msys 'objdump.exe') -p $binary.FullName |
            Select-String 'DLL Name: (.+)$' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
        foreach ($dep in $deps) {
            $source = Join-Path $msys $dep
            $target = Join-Path $dist $dep
            if ((Test-Path $source) -and -not (Test-Path $target)) {
                Copy-Item $source $target
                $added++
            }
        }
    }
} while ($added -gt 0)

$count = (Get-ChildItem $dist -Recurse -File).Count
$sizeMb = [math]::Round(((Get-ChildItem $dist -Recurse -File | Measure-Object Length -Sum).Sum / 1MB), 1)
Write-Host "Hazır: $dist ($count dosya, $sizeMb MB)"
