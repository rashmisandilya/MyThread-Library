includes = mythread.h mystruct.h
objects = mythread.o
library = mythread.a

$(library): $(objects)
	ar rcs $(library) $(objects)

$(objects): $(includes)

.PHONY: clean 
clean:
	rm -f $(library) $(objects)

