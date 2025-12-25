## Simple Makefile

# Compile tri.c
tri: tri.c
	gcc $< -o $@ 

arm: tri.c
	arm-linux-gnueabihf-gcc --static $< -o $@ 

# Run the program Eg. make run N=13
run: tri
	./tri $(N)

# Clean up binaries
clean:
	rm -f tri