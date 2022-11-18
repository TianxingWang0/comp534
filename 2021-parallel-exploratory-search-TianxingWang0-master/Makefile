NOWARN=-wd3946 -wd3947 -wd10010

EXEC=othello
OBJ =  $(EXEC) $(EXEC)-debug $(EXEC)-serial

# flags
OPT=-O2 -g $(NOWARN)
DEBUG=-O0 -g $(NOWARN)

# --- set number of workers to non-default value
ifneq ($(W),)
W=32
XX=time CILK_NWORKERS=$(W)
endif

ifneq ($(D),)
YY=cilkview.$(D)
endif


I=input

all: $(OBJ)

# build the debug parallel version of the program
$(EXEC)-debug: $(wildcard *.cpp)
	icpc $(DEBUG) -o $(EXEC)-debug $(wildcard *.cpp) -lrt


# build the serial version of the program
$(EXEC)-serial: $(wildcard *.cpp)
	icpc $(OPT) -o $(EXEC)-serial -cilk-serialize $(wildcard *.cpp) -lrt

# build the optimized parallel version of the program
$(EXEC): $(wildcard *.cpp)
	icpc $(OPT) -o $(EXEC) $(wildcard *.cpp) -lrt

#run the optimized program in parallel
runp:
	@echo use make runp W=nworkers I=input_file
	$(XX) ./$(EXEC)  < $(I)

#run the serial version of your program
runs: $(EXEC)-serial
	@echo use make runs I=input_file 
	./$(EXEC)-serial < $(I)

#run the optimized program in with cilkscreen
screen: $(EXEC)
	cilkscreen ./$(EXEC) < screen_input 2>&1 | tee cilkscreen.out

#run the optimized program in with cilkview
view: $(EXEC)
	cilkview ./$(EXEC) < $I 2>&1 | tee $(YY)


clean:
	/bin/rm -f $(OBJ) 
