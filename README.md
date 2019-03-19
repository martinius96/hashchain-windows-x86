# Hashchain for Windows x86 - 32-bit version
* Original C code from: https://github.com/brenns10/hashchain

| Libraries | Original location (change for your purposes) |
| ------------- | ------------- |
| OpenSSL v1+ | C:\BATCHES\include\openssl (includes <openssl/lib.h> |
| lcrypto dll  | C:\CodeBlocks\MinGW\lib (linked by compiler directly)|

Build (compilation.bat //MinGW x86 required)
-----
    gcc hashchain.c -o hashchain.exe -lcrypto -I C:\BATCHES\include
Create (hash_function.bat)
-----
    hashchain.exe create hash_function LENGHT "STRING FOR HASH"
Verify (hash_function.bat) - on success prints success, otherwise failure
-----
    hashchain.exe verify hash_function hash1_base64 hash2_base64
* hash1_base64(n) --> hash2_base64(n+1) - for instance (n, n+1 are base64_decoded)
* base64 encode is used only for printf
