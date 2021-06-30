# Makefile

# Version 1.0
# webserver: http_conn.o main.o
# 	g++ http_conn.o main.o -o app -pthread

# http_conn.o : http_conn.cpp
# 	g++ -c http_conn.cpp -o http_conn.o -pthread

# main.o : main.cpp
# 	g++ -c main.cpp -o main.o -pthread

# Version 1.1
# 获取目录下所有的 .cpp 文件
src = $(wildcard ./*.cpp)	
objs = $(patsubst %.cpp, %.o, $(src))
target = webserver
$(target): $(src)
	$(CXX) $(src) -o $(target) -pthread

# clean
.PHONY : clean
clean :
	rm $(objs) -f
