PROJECT=h264-rotate-render
CFLAG=-o2 -fPIC -I./include -I./include/sys/ -I./include/sys/HAL  -I./include/ff/
#LDFLAG=-L./lib -larkapi -lpthread
#LDFLAG=-L./lib -lGAL -lmfc -larkapi -lpthread
LDFLAG=-L./lib -lGAL -lmfc -larkapi -lpthread -L./lib/ff -lavformat -lavcodec -lswscale -lavutil -lx264 -ldl -lz

CXX=arm-linux-gnueabihf-g++
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))

$(warning "OBJS$(OBJS)")

$(PROJECT):$(OBJS)
	$(CXX) $^ -o $@ $(LDFLAG)
%.o:%.cpp
	$(CXX) -c $< $(CFLAG) -o $@   

clean:
	rm -f *.o $(PROJECT)
