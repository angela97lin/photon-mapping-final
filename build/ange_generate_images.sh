# This is the script we will test your submission with.

SIZE="400 400"
BIN=./a4

${BIN} -size ${SIZE} -input ../data/scene12_diffuse.txt  -output out/test.png -normals out/testn.png -depth 8 18 out/testd.png

