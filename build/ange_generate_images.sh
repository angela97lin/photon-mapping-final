# This is the script we will test your submission with.

SIZE="800 800"
BIN=./a4

${BIN} -size ${SIZE} -input ../data/scene13.txt -bounces 4 -shadows -output out/s13.png  -normals out/test13.png
${BIN} -size ${SIZE} -input ../data/scene12.txt -bounces 4 -shadows -output out/s12.png  -normals out/test12.png
