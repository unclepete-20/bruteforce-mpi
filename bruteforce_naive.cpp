#include <iostream>
#include <openssl/des.h>
#include <cstring>
#include <ctime>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <string>
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

void bruteforce(const char *ciphertext, const char *known_substring, int key_length, uint64_t max_tries, int id, int num_procs, bool &found) {
    unsigned char key[key_length] = {0};
    char encrypted_known[64];
    char decryptedtext[64];

    uint64_t range_per_node = max_tries / num_procs;
    uint64_t mylower = range_per_node * id;
    uint64_t myupper = range_per_node * (id + 1);

    // Cifra la cadena conocida una vez para comparar con el texto cifrado
    des_encrypt(key, known_substring, encrypted_known);

    for (uint64_t i = mylower; i < myupper; i++) {
        des_encrypt(key, known_substring, encrypted_known); // Cifra la cadena conocida con la clave actual
        if (strcmp(encrypted_known, ciphertext) == 0) { // Compara el resultado cifrado con el texto cifrado proporcionado
            found = true; // Establece found en true si encuentra la clave
            std::cout << "Llave encontrada por proceso " << id << ": ";
            for (int j = 0; j < key_length; j++) {
                std::cout << (int)key[j] << " ";
            }
            std::cout << std::endl;
            std::cout << "Texto descifrado: " << known_substring << std::endl;

            // Enviar un mensaje a los demás procesos para detener la búsqueda
            for (int proc = 0; proc < num_procs; proc++) {
                if (proc != id) {
                    MPI_Send(NULL, 0, MPI_INT, proc, 0, MPI_COMM_WORLD);
                }
            }
            break;
        }
        increment_key(key, key_length);

        // Verificar si se recibió un mensaje de detención
        MPI_Status status;
        int stop_signal;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &stop_signal, &status);
        if (stop_signal) {
            break;
        }
    }
}



int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int id, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc != 3) {
        if (id == 0) {
            std::cerr << "Uso: " << argv[0] << " <archivo.txt> <llave_privada>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    // Obtener la llave privada desde los argumentos de la línea de comandos como cadena
    std::string llave_privada_str = argv[2];
    const char *known_substring = "es una prueba de";
    char ciphertext[64];
    unsigned char key[8] = {0}; // Cambiado a 8 bytes para manejar números largos

    // Leer el texto desde el archivo (solo el proceso 0 lo hace)
    if (id == 0) {
        std::ifstream inputFile(argv[1]);
        if (!inputFile) {
            std::cerr << "No se pudo abrir el archivo de entrada." << std::endl;
            MPI_Finalize();
            return 1;
        }

        std::string plaintext;
        getline(inputFile, plaintext);

        // Convertir la llave privada a un número largo
        uint64_t llave_privada = std::stoull(llave_privada_str);
        
        for (int i = 0; i < 8; i++) {
            key[i] = (llave_privada >> (i * 8)) & 0xFF;
        }

        // Cifrar el texto leído desde el archivo
        des_encrypt(key, plaintext.c_str(), ciphertext);

        // Mostrar el texto cifrado
        std::cout << "Texto cifrado: " << ciphertext << std::endl;

    }
    // Broadcast la clave a todos los demás procesos
    MPI_Bcast(key, 8, MPI_CHAR, 0, MPI_COMM_WORLD);

    bool found = false;
    // Realizar el ataque de fuerza bruta para encontrar la clave
    clock_t start_time = clock();
    bruteforce(ciphertext, known_substring, 8, 1000000000, id, num_procs,found); // Cambiado a 8 bytes para manejar números largos
    clock_t end_time = clock();
    double time_taken = double(end_time - start_time) / CLOCKS_PER_SEC;

    MPI_Barrier(MPI_COMM_WORLD);

    if (found && id == 0) {
        std::cout << "\nTiempo tomado para encontrar la llave: " << time_taken << " segundos" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
