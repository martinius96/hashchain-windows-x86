# Hashchain for Windows x86 - 32-bit version
* Original C code from: https://github.com/brenns10/hashchain

| Libraries | Location (change for your purposes) |
| ------------- | ------------- |
| OpenSSL v1+ | C:\BATCHES\include\openssl (includes <openssl/lib.h> |
| lcrypto dll  | C:\MinGW\lib (linked by compiler directly)|

Build (compilation.bat //MinGW x86 required)
-----
    gcc hashchain.c -o hashchain.exe -lcrypto -I C:\BATCHES\include
Create (hash_function.bat)
-----
    hashchain.exe create hash_function LENGHT "STRING FOR HASH"
Verify (hash_function.bat)
-----
    hashchain.exe verify hash_function hash1_base64 hash2_base64
* hash1_base64(n) --> hash2_base64(hash1_base64) - for instance     
