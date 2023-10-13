#include <iostream>
#include <fstream>
#include <cstring>
#include <openssl/des.h>
#include <openssl/sha.h>
#include <mpi.h>

void des_encrypt(const DES_cblock &key, const char *plaintext, char *ciphertext) {
    DES_key_schedule keysched;
    DES_set_key_unchecked((const_DES_cblock *)&key, &keysched);
    DES_ecb_encrypt((const_DES_cblock *)plaintext, (DES_cblock *)ciphertext, &keysched, DES_ENCRYPT);
}

void des_decrypt(const DES_cblock &key, const char *ciphertext, char *plaintext) {
    DES_key_schedule keysched;
    DES_set_key_unchecked((const_DES_cblock *)&key, &keysched);
    DES_ecb_encrypt((const_DES_cblock *)ciphertext, (DES_cblock *)plaintext, &keysched, DES_DECRYPT);
}

void increment_key(DES_cblock &key) {
    // Incrementar la clave como desees, aquí se usa una simple suma
    for (int i = 0; i < 8; i++) {
        key[i]++;
        if (key[i] != 0) break;  // Verificar el desbordamiento
    }
}

void bruteforce(const char *ciphertext, const char *known_substring, int rank, int size, bool &found) {
    DES_cblock key = {0};
    char decryptedtext[8];

    uint64_t max_tries = 1ULL << 56;
    uint64_t keys_per_process = max_tries / size;
    uint64_t start = rank * keys_per_process;
    uint64_t end = (rank == size - 1) ? max_tries : (rank + 1) * keys_per_process;

    double start_time = MPI_Wtime();

    for (uint64_t i = 0; i < start; i++) {
        increment_key(key);
    }

    for (uint64_t i = start; i < end && !found; i++) {
        des_decrypt(key, ciphertext, decryptedtext);

        if (strstr(decryptedtext, known_substring) != nullptr) {
            double end_time = MPI_Wtime();
            double elapsed_time = end_time - start_time;

            std::cout << "Proceso " << rank << " encontró la clave" << std::endl;
            std::cout << "Mensaje desencriptado: " << decryptedtext << std::endl;
            std::cout << "Clave utilizada: ";
            for (int j = 0; j < 8; j++) {
                std::cout << std::hex << (int)key[j];
            }
            std::cout << std::dec << std::endl;
            std::cout << "Tiempo transcurrido: " << elapsed_time << " segundos" << std::endl;

            found = true;
            break;
        }

        increment_key(key);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const char *known_substring = "es una prueba de";

    if (argc != 3) {
        if (rank == 0) {
            std::cout << "Uso: mpirun -np 4 ./bruteforce <.txt_file> <private_key>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    const char *filename = argv[1];
    const char *private_key = argv[2];

    std::ifstream file(filename);
    if (!file.is_open()) {
        if (rank == 0) {
            std::cerr << "No se pudo abrir el archivo " << filename << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::string plaintext;
    std::string line;
    while (std::getline(file, line)) {
        plaintext += line + "\n";
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, private_key, strlen(private_key));

    DES_cblock key;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &sha256);

    // Copiar los primeros 8 bytes del resumen SHA-256 a la clave DES
    memcpy(key, digest, 8);

    char ciphertext[8];
    des_encrypt(key, plaintext.c_str(), ciphertext);

    bool found = false;
    bool global_found = false;

    bruteforce(ciphertext, known_substring, rank, size, found);
    MPI_Allreduce(&found, &global_found, 1, MPI_CXX_BOOL, MPI_LOR, MPI_COMM_WORLD);

    if (!global_found && rank == 0) {
        std::cout << "La clave no se encontró en el espacio de búsqueda." << std::endl;
    }

    file.close();
    MPI_Finalize();

    return 0;
}
