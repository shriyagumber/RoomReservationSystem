# Compiler and flags
CC=g++

# Executables
TARGETS=serverEEB serverRTH serverM serverC client

# Source files
SRC_serverEEB=serverEEB.cpp 
SRC_serverRTH=serverRTH.cpp 
SRC_serverM=serverM.cpp 
SRC_serverC=serverC.cpp 
SRC_client=client.cpp
# 'all' target
all: $(TARGETS)

# Individual targets for executables
serverEEB: $(SRC_serverEEB)
	$(CC)  -o $@ $(SRC_serverEEB)

serverRTH: $(SRC_serverRTH)
	$(CC)  -o $@ $(SRC_serverRTH)

serverM: $(SRC_serverM)
	$(CC)  -o $@ $(SRC_serverM)

serverC: $(SRC_serverC)
	$(CC)  -o $@ $(SRC_serverC)

client: $(SRC_client)
	$(CC) -o $@ $(SRC_client)

# 'clean' target
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
