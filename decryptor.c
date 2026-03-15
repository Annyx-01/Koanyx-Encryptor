#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include "dirent.h"
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#pragma comment(lib,"Crypt32.lib")

#define BUFFER_SIZE 1024
#define PATH_SEPARATOR '\\'
#define VTY_EXTENSION ".vty"

void displayErrorMessage(DWORD errorCode) {
    LPSTR messageBuffer = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL);

    if (messageBuffer != NULL) {
        printf("Error: %s\n", messageBuffer);
        LocalFree(messageBuffer);
    }
}

void printBanner() {
    printf("\n");
    printf("██╗  ██╗ ██████╗  █████╗ ███╗   ██╗██╗   ██╗██╗  ██╗\n");
    printf("██║ ██╔╝██╔═══██╗██╔══██╗████╗  ██║╚██╗ ██╔╝╚██╗██╔╝\n");
    printf("█████╔╝ ██║   ██║███████║██╔██╗ ██║ ╚████╔╝  ╚███╔╝ \n");
    printf("██╔═██╗ ██║   ██║██╔══██║██║╚██╗██║  ╚██╔╝   ██╔██╗ \n");
    printf("██║  ██╗╚██████╔╝██║  ██║██║ ╚████║   ██║   ██╔╝ ██╗\n");
    printf("╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝\n");
    printf("                  DECRYPTOR v1.0                      \n");
    printf("=======================================================\n\n");
}

void decryptFile(const char* filePath, BYTE* key, DWORD keyLen) {
    HANDLE hFile = CreateFileA(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    HCRYPTPROV hCryptProv;
    CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

    HCRYPTHASH hHash;
    CryptCreateHash(hCryptProv, CALG_SHA_256, 0, 0, &hHash);
    CryptHashData(hHash, (const BYTE*)key, keyLen, 0);

    HCRYPTKEY hKey;
    CryptDeriveKey(hCryptProv, CALG_AES_256, hHash, 0, &hKey);

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    BYTE* pBuffer = (BYTE*)malloc(dwFileSize);
    
    DWORD dwBytesRead = 0;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL);

    DWORD dwBytesToDecrypt = dwBytesRead;
    if (CryptDecrypt(hKey, 0, TRUE, 0, pBuffer, &dwBytesToDecrypt)) {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        DWORD dwBytesWritten = 0;
        WriteFile(hFile, pBuffer, dwBytesToDecrypt, &dwBytesWritten, NULL);
        SetEndOfFile(hFile);
        
        // Remove .vty extension
        char newFilePath[BUFFER_SIZE];
        strcpy(newFilePath, filePath);
        char* ext = strstr(newFilePath, VTY_EXTENSION);
        if (ext != NULL) {
            *ext = '\0';
            if (MoveFileA(filePath, newFilePath)) {
                printf("[+] Restored: %s\n", newFilePath);
            }
        }
    }

    free(pBuffer);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hCryptProv, 0);
    CloseHandle(hFile);
}

void decryptDirectory(const char* dirPath, BYTE* key, DWORD keyLen) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
    char fullPath[BUFFER_SIZE];

    if (!(dir = opendir(dirPath))) return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(fullPath, sizeof(fullPath), "%s%c%s", dirPath, PATH_SEPARATOR, entry->d_name);
        
        if (stat(fullPath, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                decryptDirectory(fullPath, key, keyLen);
            } else {
                if (strstr(entry->d_name, VTY_EXTENSION) != NULL) {
                    decryptFile(fullPath, key, keyLen);
                }
            }
        }
    }
    closedir(dir);
}

BYTE* decryptAES(const char* aesKeyFile, const char* privKeyFile, DWORD* keyLen) {
    HANDLE hPrivKey = CreateFileA(privKeyFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPrivKey == INVALID_HANDLE_VALUE) {
        printf("[-] Error: private_key.pem not found!\n");
        return NULL;
    }
    
    DWORD privKeySize = GetFileSize(hPrivKey, NULL);
    BYTE* privKeyData = (BYTE*)malloc(privKeySize);
    
    DWORD read = 0;
    ReadFile(hPrivKey, privKeyData, privKeySize, &read, NULL);
    CloseHandle(hPrivKey);

    HCRYPTPROV hProv;
    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

    DWORD privKeyBufSize = 0;
    CryptStringToBinaryA((LPCSTR)privKeyData, privKeySize, CRYPT_STRING_BASE64HEADER, NULL, &privKeyBufSize, NULL, NULL);

    BYTE* privKeyBuf = (BYTE*)malloc(privKeyBufSize);
    CryptStringToBinaryA((LPCSTR)privKeyData, privKeySize, CRYPT_STRING_BASE64HEADER, privKeyBuf, &privKeyBufSize, NULL, NULL);
    free(privKeyData);

    DWORD derSize = 0;
    CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, privKeyBuf, privKeyBufSize, CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &derSize);

    BYTE* privKeyBlob = (BYTE*)malloc(derSize);
    CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, privKeyBuf, privKeyBufSize, CRYPT_DECODE_ALLOC_FLAG, NULL, privKeyBlob, &derSize);
    free(privKeyBuf);

    HCRYPTKEY hPrivKeyCrypt;
    CryptImportKey(hProv, privKeyBlob, derSize, 0, CRYPT_EXPORTABLE, &hPrivKeyCrypt);
    free(privKeyBlob);

    HANDLE hAES = CreateFileA(aesKeyFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hAES == INVALID_HANDLE_VALUE) {
        printf("[-] Error: encrypted_aes_key.bin not found!\n");
        CryptDestroyKey(hPrivKeyCrypt);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }
    
    DWORD aesSize = GetFileSize(hAES, NULL);
    BYTE* encAESKey = (BYTE*)malloc(aesSize);
    
    ReadFile(hAES, encAESKey, aesSize, &read, NULL);
    CloseHandle(hAES);

    DWORD decLen = aesSize;
    if (!CryptDecrypt(hPrivKeyCrypt, 0, TRUE, 0, encAESKey, &decLen)) {
        printf("[-] Failed to decrypt AES key! Wrong private key?\n");
        free(encAESKey);
        CryptDestroyKey(hPrivKeyCrypt);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    *keyLen = decLen;
    printf("[+] AES key successfully decrypted\n");

    CryptDestroyKey(hPrivKeyCrypt);
    CryptReleaseContext(hProv, 0);
    
    return encAESKey;
}

int main() {
    printBanner();
    
    DWORD aesKeyLen = 0;
    BYTE* aesKey = decryptAES("C:\\encrypted_aes_key.bin", "C:\\private_key.pem", &aesKeyLen);
    
    if (aesKey == NULL) {
        printf("\n[-] DECRYPTION FAILED!\n");
        printf("[-] Make sure private_key.pem matches the public key used\n");
        system("pause");
        return 1;
    }
    
    printf("\n[*] Starting decryption of ALL drives...\n\n");
    
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            rootPath[0] = 'A' + i;
            printf("[*] Decrypting drive: %s\n", rootPath);
            decryptDirectory(rootPath, aesKey, aesKeyLen);
        }
    }
    
    free(aesKey);
    
    printf("\n========================================\n");
    printf("[+] DECRYPTION COMPLETE!\n");
    printf("[+] All files have been restored\n");
    printf("========================================\n\n");
    
    system("pause");
    return 0;
}