run:
#sh:
	g++ shell.cpp  -o shell

#tr:
	g++ -o trojan trojan.cpp
#sqb:
	g++ -o sb squashbug.cpp
	sudo cp sb /usr/local/bin/
#fl:
	g++ -o filelocker filelocker.cpp
#del:
	g++ -o delep delep.cpp
	sudo cp delep /usr/local/bin/

clean:
	rm -f trojan sb delep filelocker
	sudo rm -f shell /usr/local/bin/delep /usr/local/bin/sb