dir=/home/aaji/softs/boost
    
all: map tmap

map: imageMapper.cpp
	g++ -m64 -O3 imageMapper.cpp -o iMapper -I${dir}/include -L${dir}/lib

tmap: tileMapper.cpp
	g++ -m64 -O3 tileMapper.cpp -o tMapper -I${dir}/include -L${dir}/lib

clean:
	rm -f iMap
	rm -f tMap

