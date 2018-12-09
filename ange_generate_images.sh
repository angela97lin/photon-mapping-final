# This is the script we will test your submission with.

SIZE="500 500"
BIN=./a4


${BIN} -size ${SIZE} -input ../data/scene13.txt -bounces 4 -shadows -output out/a13.png
