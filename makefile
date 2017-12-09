#-- program name, sources and obj --
TARGET=Appli
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:.cpp=.o)

#-- location where SFML header files and libraries are installed --
SFMLDIR=/home/TP/modules/sujets/SFML-2.1

#-- compiler/linker settings --
CXX=$(wildcard /opt/rh/devtoolset-2/root/usr/bin/g++)
ifeq (${CXX},)
  CXX=g++
endif
CXXFLAGS= -std=c++14 -W -Wall -Wno-long-long -pedantic -Wextra -Wconversion -g -O4 -pthread
IFLAGS= -I$(SFMLDIR)/include -I./include
LDFLAGS= -L$(SFMLDIR)/lib -lsfml-graphics -lsfml-window -lsfml-system -lpthread \
        -Wl,-rpath,${SFMLDIR}/lib

#-- main target to obtain --
all : $(TARGET)


#-- linker command to produce the executable file --
$(TARGET) : $(OBJ)
	@echo Linking $@ ...
	$(CXX) $^ -o $@ $(LDFLAGS) 
	@echo

#-- source file header files dependencies --
src/Appli.o : include/Pong.h
src/Pong.o : include/Pong.h include/Case.h include/crsUtils.h
src/Forme.o : include/Forme.h include/Couleur.h
src/Case.o : include/Case.h include/Forme.h include/Couleur.h
src/save_datas.o : include/save_datas.h
src/crsUtis.o : include/crsUtils.h

#-- compiler command for every source file --
%.o : %.cpp 
	@echo Compiling $< ....
	$(CXX) -c $< $(IFLAGS) -o $@ $(CXXFLAGS) 
	@echo

.PHONY: clean

#-- system-specific command to remove those files --
clean : 
	@echo Cleaning...
	rm -f $(OBJ) $(TARGET) src/*~ include/*~ *~
	@echo

run :
	./$(TARGET)



