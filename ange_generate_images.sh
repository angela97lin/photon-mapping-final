# This is the script we will test your submission with.

SIZE="800 800"
BIN=./a4


${BIN} -size ${SIZE} -input ../data/scene09.txt -bounces 4 -shadows -output out/a09.png
