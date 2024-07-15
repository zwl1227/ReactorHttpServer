src = $(wildcard ./src/*.cpp)

server: $(src)
	g++ $(src)  -o server -lpthread -I./include -L ./libs -ljsoncpp -I/usr/include/mysql  -L/usr/lib/mysql -lmysqlclient  -lz -lm
