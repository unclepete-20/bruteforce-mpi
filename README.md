# bruteforce-mpi

## bruteforce_naice.cpp
compilar

g++ -o bruteforce_naive bruteforce_naive.cpp -lssl -lcrypto

correr

./bruteforce_naive
# bruteforce_naice_mpi.cpp
compilar

mpic++ -o bruteforce_mpi bruteforce_naive_mpi.cpp -lcrypto

correr

mpirun -np 4 ./bruteforce_mpi ./text <Llave_privada>
