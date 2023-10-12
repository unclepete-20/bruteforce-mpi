#include <iostream>
#include <fstream>
#include <openssl/des.h>
#include <cstring>
#include <ctime>
#include <mpi.h>

void des_encrypt(const unsigned char *key, const char *plaintext, char *ciphertext) {
    std::cout << "Clave en des_encrypt: ";
    for (int i = 0; i < 8; i++) {
        std::cout << (int)key[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Texto plano en des_encrypt: " << plaintext << std::endl;
    DES_key_schedule keysched;
    DES_cblock des_key;
    memcpy(des_key, key, 64);
    // DES_string_to_key((const char *)key, &des_key);
    DES_set_key_unchecked(&des_key, &keysched);
    std::cout << "Clave después de la configuración: ";
    for (int i = 0; i < 8; i++) {
        std::cout << (int)des_key[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "keysched antes de la encriptación: ";
    for (int i = 0; i < 16; i++) {
        std::cout << (int)keysched[i] << " ";
    }
    std::cout << std::endl;

    
    DES_ecb_encrypt((const_DES_cblock *)plaintext, (DES_cblock *)ciphertext, &keysched, DES_ENCRYPT);
}

void des_decrypt(const unsigned char *key, const char *ciphertext, char *plaintext) {
    // for (int i = 0; i < 8; i++) {
    //     std::cout << (int)key[i] << " ";
    // }
    // std::cout << std::endl;

    // std::cout << "Texto plano en des_decrypt: " << plaintext << std::endl;
    // std::cout << "ciphertext en des_decrypt: " << ciphertext << std::endl;
    DES_key_schedule keysched;
    DES_cblock des_key;
    memcpy(des_key, key, 64);
    // DES_string_to_key((const char *)key, &des_key);
    DES_set_key_unchecked(&des_key, &keysched);
    // std::cout << "Clave después de la configuración: ";
    // for (int i = 0; i < 8; i++) {
    //     std::cout << (int)des_key[i] << " ";
    // }
    // std::cout << std::endl;
    DES_ecb_encrypt((const_DES_cblock *)ciphertext, (DES_cblock *)plaintext, &keysched, DES_DECRYPT);
    // std::cout << "ciphertext en des_decrypt final: " << ciphertext << std::endl;
}

void increment_key(unsigned char key[], int key_length, uint64_t increment) {
    uint64_t carry = increment;
    for (int i = key_length - 1; i >= 0 && carry > 0; i--) {
        carry += key[i];
        key[i] = carry & 0xFF;
        carry >>= 8;
    }
}


void bruteforce(const char *ciphertext, const char *known_substring, int key_length, int rank, int size, bool &found, uint64_t start, uint64_t end) {
    char decryptedtext[8];
    unsigned char key[key_length+1];

    // std::cout << "Ciphertext recibido en bruteforce: " << ciphertext << std::endl;

    
    // std::cout << "Proceso " << rank << ": probará desde la llave " << start << " al " << end - 1 << std::endl;

    uint64_t step = size;  // Este es el paso (stride) entre las claves probadas

    for (uint64_t i = start; i < end && !found; i ++) {
        // Genera la clave como una cadena de caracteres
        char key_str[key_length];
        snprintf(key_str, key_length + 1, "%0*llu", key_length, i);
        // std::cout << "Key_str antes de copia en brutefoce: " << key_str << std::endl;

        // Copia la clave generada al array 'key' para desencriptar
        strncpy((char *)key, key_str, key_length);

        // std::cout << "Key después de strncpy en bruteforce: ";
        // for (int i = 0; i < key_length; i++) {
        //     std::cout << (int)key[i] << " ";
        // }
        // std::cout << std::endl;
    
        des_decrypt(key, ciphertext, decryptedtext);
        // std::cout << "known_substring en bruteforce: " << known_substring << std::endl;
        // std::cout << "Texto en proceso descifrado en bruteforce: " << decryptedtext << std::endl;
        if (strstr(decryptedtext, known_substring) != nullptr) {
            std::cout << "\nLlave encontrada por proceso " << rank << ": " << key_str << std::endl;
            std::cout << "Texto descifrado: " << decryptedtext << std::endl;
            found = true;
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const char *known_substring = "una";

    if (argc != 3) {
        if (rank == 0) {
            std::cout << "Uso: mpirun -np 4 ./bruteforce_naive <.txt_file> <private_key>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    const char *filename = argv[1];
    const char *private_key = argv[2];
    int key_length = strlen(private_key);
    // int key_length = 8;

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

    std::cout << "Valor de private_key: " << private_key << std::endl;
    std::cout << "Plaintext antes de encriptar: " << plaintext << std::endl;

    unsigned char key[key_length];
    strncpy((char *)key, private_key, key_length);
    std::cout << "Key después de strncpy: ";
    for (int i = 0; i < key_length; i++) {
        std::cout << (int)key[i] << " ";
    }
    std::cout << std::endl;

    char ciphertext[8];
    if (rank == 0) {
        des_encrypt(key, plaintext.c_str(), ciphertext);
        // std::cout << "Ciphertext después de encriptar: " << ciphertext << std::endl;
    }
    MPI_Bcast(ciphertext, 8, MPI_CHAR, 0, MPI_COMM_WORLD);
    // des_encrypt(key, plaintext.c_str(), ciphertext);

    std::cout << "Ciphertext después de encriptar: " << ciphertext << std::endl;

    bool found = false;
    bool global_found = false;

    // Ajustar el espacio de búsqueda para ser 10^key_length
    uint64_t max_tries = 1;
    for (int i = 0; i < key_length; i++) {
        max_tries *= 10;
    }

    uint64_t keys_per_process = max_tries / size;
    uint64_t start = rank * keys_per_process;
    uint64_t end = (rank == size - 1) ? max_tries : (rank + 1) * keys_per_process;

    // MPI_Bcast(ciphertext, 64, MPI_CHAR, 0, MPI_COMM_WORLD);

    bruteforce(ciphertext, known_substring, key_length, rank, size, found, start, end);
    MPI_Allreduce(&found, &global_found, 1, MPI_CXX_BOOL, MPI_LOR, MPI_COMM_WORLD);

    if (!global_found && rank == 0) {
        std::cout << "La clave no se encontró en el espacio de búsqueda." << std::endl;
    }

    file.close();
    MPI_Finalize();

    return 0;
}
