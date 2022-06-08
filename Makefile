mmy: iosched.cpp
	bash -c "module load gcc-9.2"
	g++ -std=c++11 -g iosched.cpp -o iosched

clean:
	rm -f iosched *~