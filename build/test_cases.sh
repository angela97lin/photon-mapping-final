#!/bin/sh

./a4 -input ../data/scene01_plane.txt -output 01.png -size 1200 1200
./a4 -input ../data/scene02_cube.txt -output 02.png -size 1200 1200
./a4 -input ../data/scene03_sphere.txt -output 03.png -size 1200 1200
./a4 -input ../data/scene04_axes.txt -output 04.png -size 1200 1200
./a4 -input ../data/scene05_bunny_200.txt -output 05.png -size 1200 1200
./a4 -input ../data/scene06_bunny_1k.txt -output 06.png -size 1300 1300 -shadows -bounces 4
./a4 -input ../data/scene07_arch.txt -output 07.png -size 1300 1300 -shadows -bounces 4 
