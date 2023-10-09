#include <iostream>
#include <fstream>
#include <openssl/des.h>
#include <cstring>
#include <ctime>
#include <mpi.h>

void des_encrypt(const unsigned char *key, const char *plaintext, char *ciphertext) {
    DES_key_schedule keysched;
    DES_cblock des_key;
    DES_string_to_key((const char *)key, &des_key);
    DES_set_key_unchecked(&des_key, &keysched);
    DES_ecb_encrypt((const_DES_cblock *)plaintext, (DES_cblock *)ciphertext, &keysched, DES_ENCRYPT);
}

void des_decrypt(const unsigned char *key, const char *ciphertext, char *plaintext) {
    DES_key_schedule keysched;
    DES_cblock des_key;
    DES_string_to_key((const char *)key, &des_key);
    DES_set_key_unchecked(&des_key, &keysched);
    DES_ecb_encrypt((const_DES_cblock *)ciphertext, (DES_cblock *)plaintext, &keysched, DES_DECRYPT);
}

void increment_key(unsigned char key[], int key_length) {
    for (int i = key_length - 1; i >= 0; i--) {
        key[i]++;
        if (key[i] != 0) {
            break;
        }
    }
}

// void bruteforce(const char *ciphertext, const char *known_substring, int key_length, uint64_t max_tries, int rank, int size, bool &found) {
//     unsigned char key[key_length] = {0};
//     char decryptedtext[64];
//     uint64_t start_try = rank;
//     uint64_t end_try = start_try + (max_tries / size);

//     for (uint64_t i = start_try; i < end_try; i ++) { // Distribuir el trabajo entre los procesos
//         des_decrypt(key, ciphertext, decryptedtext);

//         if (strstr(decryptedtext, known_substring) != nullptr) {
//             std::cout << "\nLlave encontrada por proceso " << rank << ": ";
//             for (int j = 0; j < key_length; j++) {
//                 std::cout << (int)key[j] << " ";
//             }
//             std::cout << std::endl;
//             std::cout << "Texto descifrado: " << decryptedtext << std::endl;
//             found = true; // Establecer la variable de control en true
//             break;
//         }
//         increment_key(key, key_length);
//     }
// }


bool check_key(const char *ciphertext, const char *known_substring, unsigned char *key, int key_length, int rank, bool &found, double start_time) {
    char decryptedtext[64];
    des_decrypt(key, ciphertext, decryptedtext);

    if (strstr(decryptedtext, known_substring) != nullptr) {
        std::cout << "\nLlave encontrada por proceso " << rank << ": ";
        for (int j = 0; j < key_length; j++) {
            std::cout << (int)key[j] << " ";
        }
        std::cout << std::endl;
        std::cout << "Texto descifrado: " << decryptedtext << std::endl;
        double end_time = MPI_Wtime();
        double min_execution_time = end_time - start_time;

        std::cout << "Tiempo del proceso " << rank << ": " << min_execution_time << " segundos" << std::endl;
        found = true;
    }
    return found;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const char *known_substring = "pru";

    if (argc != 3) {
        if (rank == 0) {
            std::cout << "Uso: mpirun -np 4 ./bruteforce_naive archivo.txt 123456L" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    const char *filename = argv[1];
    const char *private_key = argv[2];
    int key_length = 4;

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

    unsigned char key[key_length];
    strncpy((char *)key, private_key, key_length);
    char ciphertext[64];
    double start_time = MPI_Wtime();
    des_encrypt(key, plaintext.c_str(), ciphertext);

    bool found = false;
    bool global_found = false;

    for (uint64_t i = rank; i < 1000000000000 && !global_found; i += size) {
        check_key(ciphertext, known_substring, key, key_length, rank, found,start_time);
        increment_key(key, key_length);

        MPI_Allreduce(&found, &global_found, 1, MPI_CXX_BOOL, MPI_LOR, MPI_COMM_WORLD);

        if (global_found) {
            break;
        }
    }

    file.close();
    MPI_Finalize();

    return 0;
}
