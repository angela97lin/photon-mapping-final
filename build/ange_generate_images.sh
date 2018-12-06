# This is the script we will test your submission with.

SIZE="400 400"
BIN=./a4


${BIN} -size ${SIZE} -input ../data/scene09.txt -bounces 4 -shadows -output out/a09.png
