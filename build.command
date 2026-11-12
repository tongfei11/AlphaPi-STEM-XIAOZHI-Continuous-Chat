#!/bin/zsh
set -eu
cd "${0:A:h}"
command -v arduino-cli >/dev/null
mkdir -p .build/arduino
python3 - <<'PY'
from pathlib import Path
root = Path.cwd() / '.build/arduino'
(root / 'arduino-cli.yaml').write_text(f'''board_manager:
  additional_urls:
    - https://espressif.github.io/arduino-esp32/package_esp32_index.json
directories:
  data: {root}/data
  downloads: {root}/downloads
  user: {root}/user
''')
PY
task_config="$PWD/.build/arduino/arduino-cli.yaml"
task_version="$(sed -n 's/^#define XZ_FW_TAG " local-\([0-9][0-9][0-9][0-9]\)"$/\1/p' AlphaPi_Xiaozhi.ino)"
[[ -n "$task_version" ]]
if [[ ! -f .build/arduino/data/package_esp32_index.json || ! -f .build/arduino/data/library_index.json ]]; then
  arduino-cli --config-file "$task_config" core update-index
fi
arduino-cli --config-file "$task_config" core install esp32:esp32@2.0.14
arduino-cli --config-file "$task_config" lib install 'WebSockets@2.7.2'
if [[ ! -f .build/arduino/user/libraries/arduino-libopus-1.3.2/library.properties ]]; then
  curl -fL --retry 2 \
    'https://codeload.github.com/pschatzmann/arduino-libopus/zip/refs/tags/v1.3.2' \
    -o .build/arduino/libopus-v1.3.2.zip
  mkdir -p .build/arduino/user/libraries
  unzip -q .build/arduino/libopus-v1.3.2.zip -d .build/arduino/user/libraries
fi
task_sketch="$PWD/.build/sketch/AlphaPi_Xiaozhi"
mkdir -p "$task_sketch"
cp AlphaPi_Xiaozhi.ino TalkControls.h MicLevels.h build_opt.h "$task_sketch/"
# The ESP32 Arduino core applies the sketch's build_opt.h to Opus as well.
arduino-cli --config-file "$task_config" compile \
  --fqbn 'esp32:esp32:esp32c3:FlashSize=4M,PartitionScheme=huge_app' \
  --build-path "$PWD/.build/firmware/$task_version" \
  "$task_sketch"
python3 - "$task_version" <<'PY'
from pathlib import Path
import os
import stat
import sys
version = sys.argv[1]
output = Path('output') / version
output.mkdir(parents=True, exist_ok=True)
target = output / f'AlphaPi-Xiaozhi-{version}.bin'
target.write_bytes((Path('.build/firmware') / version / 'AlphaPi_Xiaozhi.ino.bin').read_bytes())
for path in (output.parent, output, target):
    os.chflags(path, path.stat().st_flags & ~stat.UF_HIDDEN)
print(f'可刷固件：{target.resolve()}')
PY
