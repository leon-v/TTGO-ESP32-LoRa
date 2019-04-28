#/bin/sh

if [$2 = ""]; then
	set "$1" "5"
fi

./idf.sh $1 $2 $3