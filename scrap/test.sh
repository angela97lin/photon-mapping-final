# This is the script we will test your submission with.

SIZE="800 800"
BIN=./a4

${BIN} -size ${SIZE} -input ../data/scene08_test.txt  -output out/test.png -normals out/testn.png -depth 8 18 out/testd.png

