/*
    Autor: Pedro Pablo Arriola Jimenez
    Carnet: 20188
    Descripción: Implementación paralela de un ataque de fuerza bruta sobre un texto cifrado con AES-128.
                 Utiliza MPI para distribuir la carga de trabajo entre diferentes procesos y aplica el algoritmo 
                 de búsqueda de Boyer-Moore para optimizar la identificación de substrings en texto descifrado.
*/

#include <iostream>
#include <fstream>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <cstring>
#include <ctime>
#include <mpi.h>

// Función que ejecuta el cifrado AES.
void aes_encrypt(const unsigned char *key, const unsigned char *plaintext, unsigned char *ciphertext, size_t length) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(plaintext, ciphertext, &aes_key);
}

// Función que ejecuta el descifrado AES.
void aes_decrypt(const unsigned char *key, const unsigned char *ciphertext, unsigned char *plaintext, size_t length) {
    AES_KEY aes_key;
    AES_set_decrypt_key(key, 128, &aes_key);
    AES_decrypt(ciphertext, plaintext, &aes_key);
}

// Función que incrementa el valor de la clave en 1.
void increment_key(unsigned char key[], int key_length) {
    for (int i = key_length - 1; i >= 0; i--) {
        key[i]++;
        if (key[i] != 0) {
            break;
        }
    }
}

// Precomputa los shifts necesarios para el algoritmo de Boyer-Moore.
void precompute_shifts(const unsigned char* pattern, size_t pat_len, std::vector<size_t>& shifts) {
    const size_t ALPHABET_SIZE = 256;
    shifts = std::vector<size_t>(ALPHABET_SIZE, pat_len);
    for (size_t i = 0; i < pat_len - 1; i++) {
        shifts[pattern[i]] = pat_len - 1 - i;
    }
}

// Implementación de la búsqueda de Boyer-Moore.
bool boyer_moore_search(const unsigned char* text, size_t txt_len, const unsigned char* pattern, size_t pat_len) {
    std::vector<size_t> shifts;
    precompute_shifts(pattern, pat_len, shifts);

    size_t i = 0;
    while (i <= txt_len - pat_len) {
        size_t j = pat_len - 1;
        while (j < pat_len && pattern[j] == text[i + j]) {
            j--;
        }
        if (j >= pat_len) {
            return true;
        } else {
            i += shifts[text[i + pat_len - 1]];
        }
    }
    return false;
}

// Método que realiza el ataque de fuerza bruta para intentar descifrar el texto.
void bruteforce(const unsigned char *ciphertext, const unsigned char *known_substring, int key_length, uint64_t max_tries, int rank, int size) {
    unsigned char key[16] = {0};
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

        aes_decrypt(key, ciphertext, decryptedtext, sizeof(ciphertext));

        if (strstr(reinterpret_cast<const char*>(decryptedtext), reinterpret_cast<const char*>(known_substring)) != nullptr) {
            std::cout << "\nKey Found by Rank " << rank << ": ";
            for (int j = 0; j < key_length; j++) {
                std::cout << (int)key[j] << " ";
            }
            std::cout << std::endl;
            std::cout << "Decrypted Text: " << decryptedtext << std::endl;

            int flag = 1;
            for (int p = 0; p < size; p++) {
                if (p != rank) {
                    MPI_Send(&flag, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                }
            }

            break;
        }

        increment_key(key, key_length);

        MPI_Status status;
        int recv_flag;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_flag, &status);
        if (recv_flag) {
            MPI_Recv(&recv_flag, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            break;
        }
    }
}

// Método principal del programa.
int main(int argc, char *argv[]) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    unsigned char ciphertext[64] = "your_ciphertext_here";  // Se debe reemplazar por el texto cifrado real.
    unsigned char known_substring[64] = "known_text_here";   // Se debe reemplazar por el texto conocido.
    int key_length = 16;  // Longitud de la clave para AES-128.
    uint64_t max_tries = UINT64_MAX;  // Máximo número de intentos (todas las posibles claves).

    bruteforce(ciphertext, known_substring, key_length, max_tries, rank, size);

    MPI_Finalize();

    return 0;
}

