#/bin/sh

if [$2 = ""]; then
	set "$1" "6"
fi

./idf.sh $1 $2 $3