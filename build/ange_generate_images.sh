# This is the script we will test your submission with.

SIZE="500 500"
BIN=./a4


${BIN} -size ${SIZE} -input ../data/scene12_diffusered.txt  -output out/test_red.png -normals out/test_red_map.png -depth 8 18 out/test_red_original.png

${BIN} -size ${SIZE} -input ../data/scene12_dcolor.txt  -output out/test_color.png -normals out/test_map_color.png -depth 8 18 out/test_original_color.png

${BIN} -size ${SIZE} -input ../data/scene12_diffuse.txt  -output out/test.png -normals out/test_map.png -depth 8 18 out/test_original.png

