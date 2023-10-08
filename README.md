# bruteforce-mpi

compilar
mpic++ -o bruteforce_naive bruteforce_naive.cpp -lcrypto

correr
mpirun -np 4 ./bruteforce_naive ./text <Llave_privada>
