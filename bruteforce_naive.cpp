#include <iostream>
#include <openssl/des.h>
#include <cstring>
#include <ctime>

void des_encrypt(const unsigned char *key, const char *plaintext, char *ciphertext) {
    DES_key_schedule keysched;
    DES_set_key_unchecked((const_DES_cblock *)key, &keysched);
    DES_ecb_encrypt((const_DES_cblock *)plaintext, (DES_cblock *)ciphertext, &keysched, DES_ENCRYPT);
}

void des_decrypt(const unsigned char *key, const char *ciphertext, char *plaintext) {
    DES_key_schedule keysched;
    DES_set_key_unchecked((const_DES_cblock *)key, &keysched);
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

void bruteforce(const char *ciphertext, const char *known_substring, int key_length, uint64_t max_tries) {
    unsigned char key[key_length] = {0};
    char decryptedtext[64];

    for (uint64_t i = 0; i < max_tries; i++) {
        des_decrypt(key, ciphertext, decryptedtext);
        std::cout << "Llave Probada: ";
        for (int j = 0; j < key_length; j++) {
            std::cout << (int)key[j] << " ";
        }
        std::cout << " - Texto Descifrado: " << decryptedtext << std::endl;

        if (strstr(decryptedtext, known_substring) != nullptr) {
            std::cout << "\nLlave encontrada: ";
            for (int j = 0; j < key_length; j++) {
                std::cout << (int)key[j] << " ";
            }
            std::cout << std::endl;
            std::cout << "Texto descifrado: " << decryptedtext << std::endl;
            break;
        }
        increment_key(key, key_length);
    }
}

int main() {
    unsigned char key[1] = {123};
    const char *plaintext = "HOLA";
    const char *known_substring = "HO";
    char ciphertext[64];

    des_encrypt(key, plaintext, ciphertext);
    clock_t start_time = clock();
    bruteforce(ciphertext, known_substring, 1, 1000000000);
    clock_t end_time = clock();
    double time_taken = double(end_time - start_time) / CLOCKS_PER_SEC;
    std::cout << "\nTiempo tomado para encontrar la llave: " << time_taken << " segundos" << std::endl;

    return 0;
}




