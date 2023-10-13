#include <iostream>
#include <fstream>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <cstring>
#include <ctime>
#include <mpi.h>

void aes_encrypt(const unsigned char *key, const unsigned char *plaintext, unsigned char *ciphertext, size_t length) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(plaintext, ciphertext, &aes_key);
}

void aes_decrypt(const unsigned char *key, const unsigned char *ciphertext, unsigned char *plaintext, size_t length) {
    AES_KEY aes_key;
    AES_set_decrypt_key(key, 128, &aes_key);
    AES_decrypt(ciphertext, plaintext, &aes_key);
}

void increment_key(unsigned char key[], int key_length) {
    for (int i = key_length - 1; i >= 0; i--) {
        key[i]++;
        if (key[i] != 0) {
            break;
        }
    }
}

void bruteforce(const unsigned char *ciphertext, const unsigned char *known_substring, int key_length, uint64_t max_tries, int rank, int size) {
    unsigned char key[16] = {0}; // AES-128 key size
    unsigned char decryptedtext[64];

    int chunk = max_tries / size;
    int remainder = max_tries % size;

    if (rank < remainder) {
        chunk++;
    }

    int start = rank * chunk;
    int end = start + chunk;

    std::cout << "Rank " << rank << " starting from " << start << " to " << end << std::endl;

    for (uint64_t i = start; i < end; i++) {
        // Imprimir la clave que se está probando
        // std::cout << "Rank " << rank << " trying key: ";
        // for (int j = 0; j < key_length; j++) {
        //     std::cout << (int)key[j] << " ";
        // }
        // std::cout << std::endl;

        aes_decrypt(key, ciphertext, decryptedtext, sizeof(ciphertext));

        if (strstr(reinterpret_cast<const char*>(decryptedtext), reinterpret_cast<const char*>(known_substring)) != nullptr) {
            std::cout << "\nKey Found by Rank " << rank << ": ";
            for (int j = 0; j < key_length; j++) {
                std::cout << (int)key[j] << " ";
            }
            std::cout << std::endl;
            std::cout << "Decrypted Text: " << decryptedtext << std::endl;

            // Signal other processes to stop
            int flag = 1;
            for (int p = 0; p < size; p++) {
                if (p != rank) {
                    MPI_Send(&flag, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                }
            }

            break;
        }

        increment_key(key, key_length);

        // Check for stop signal from other processes
        MPI_Status status;
        int recv_flag;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_flag, &status);
        if (recv_flag) {
            MPI_Recv(&recv_flag, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
            std::cout << "Rank " << rank << " received stop signal from Rank " << status.MPI_SOURCE << std::endl;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    unsigned char key[16] = {0}; // AES-128 key size
    const char *known_substring = "prue";
    unsigned char ciphertext[64];

    // Check if a filename argument is provided
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.txt> <private_key>" << std::endl;
        MPI_Finalize();
        return 1;
    }

    const char *filename = argv[1];
    std::ifstream file(filename);

    const char *private_key_str = argv[2];
    unsigned char private_key[16] = {0};

    if (strlen(private_key_str) > 16) {
        // Si la clave privada es más larga que 16 bytes, calcula un hash SHA-256
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)private_key_str, strlen(private_key_str), hash);

        // Copia los primeros 16 bytes del hash como la clave privada
        for (int i = 0; i < 16; i++) {
            private_key[i] = hash[i];
        }
    } else {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)private_key_str, strlen(private_key_str), hash);

        // Copia los primeros 16 bytes del hash como la clave privada
        for (int i = 0; i < 16; i++) {
            private_key[i] = hash[i];
        }
    }

    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        MPI_Finalize();
        return 1;
    }

    std::cout << "Private Key: ";
    for (int i = 0; i < 16; i++) {
        std::cout << (int)private_key[i] << " ";
    }
    std::cout << std::dec << std::endl;


    std::string plaintext((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Calculate the length of the plaintext
    size_t plaintext_length = plaintext.size();

    // Initialize the ciphertext with zeros
    memset(ciphertext, 0, sizeof(ciphertext));

    // Encrypt the entire plaintext
    aes_encrypt(private_key, reinterpret_cast<const unsigned char*>(plaintext.c_str()), ciphertext, plaintext_length);

    clock_t start_time = clock();

    // Call the bruteforce function with the updated length
    bruteforce(ciphertext, reinterpret_cast<const unsigned char*>(known_substring), 16, 1000000000, rank, size);

    clock_t end_time = clock();
    double time_taken = double(end_time - start_time) / CLOCKS_PER_SEC;
    std::cout << "\nTime taken by Rank " << rank << " to find the key: " << time_taken << " seconds" << std::endl;

    MPI_Finalize();
    return 0;
}
