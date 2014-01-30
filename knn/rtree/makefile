dir=/home/aaji/softs

all: mapper imageReducer tileReducer

mapper: imageMapper.cpp tileMapper.cpp
	g++ -m64 -O3 imageMapper.cpp -o iMapper -I${dir}/boost/include -L${dir}/boost/lib
	g++ -m64 -O3 tileMapper.cpp  -o tMapper -I${dir}/boost/include -L${dir}/boost/lib

imageReducer: imageReducer.cpp IndexParam.h Reducer.h
	g++ -O3 -m64 imageReducer.cpp -o iReducer -lspatialindex -I${dir}/include -L${dir}/lib

tileReducer: tileReducer.cpp IndexParam.h Reducer.h
	g++ -O3 -m64 tileReducer.cpp -o tReducer -lspatialindex -I${dir}/include -L${dir}/lib

clean:
	rm -f iReducer
	rm -f tReducer
	rm -f tMapper
	rm -f iMapper
