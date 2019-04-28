#/bin/sh

export IDF_PATH=$PWD/esp-idf
export PATH=$IDF_PATH/tools:$PATH
export PATH=$PWD/toolchain/xtensa-esp32-elf/bin/:$PATH


if [$2 = ""]; then
	set "$1" "4"
fi

idf.py $1 -p /dev/ttyS$2 -b 115200 $3