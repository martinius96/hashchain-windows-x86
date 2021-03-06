/***************************************************************************//**

  @file       hashchain.c

  @author     Stephen Brennan

  @date       Created Sunday, June 19, 2016

  @brief      Command line hash chain tool.

  @copyright  Copyright (c) 2016 Stephen Brennan.
              Released under Revised BSD License.

  The idea of a hash chain is simple: you start with a base (could be a
  password, or just a number, or some other data) and hash it. You then take the
  result and hash that too. You continue hashing the results repeatedly, till
  you have a series of hashes like this:

  Base -> Hash0 = H(Base) -> Hash1 = H(Hash0) -> ... -> HashN = H(HashN-1)

  The exciting property of these hash chains is that given the last hash in the
  chain, HashN, it's very difficult (read: impossible) to determine any of the
  previous hashes, or the base. However, given the last hash, it is trivial to
  verify whether another hash is part of the chain.

  This means that a hash chain has the potential to be a limited source of
  authentication. You can deploy a resource in public along with the last hash
  of the chain. Then you can give commands to this resource, passing along each
  previous hash as authentication of your identity.

*******************************************************************************/

#include <inttypes.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
   @brief Represents a hash chain.

   Everything here is required to be able to comprehend a hash chain. The
   digest_size tells us how big each hash is, so that we can index properly. The
   chain_length tells us how many hashes there are. The data pointer points at a
   buffer of length chain_length * digest_size.
 */
struct hash_chain {
  int digest_size;
  int chain_length;
  uint8_t *data;
};

/**
   @brief Generate and return a hash chain.
   @param base Pointer to seed data for first hash.
   @param baselen Number of bytes in seed data.
   @param type The hash algorithm to use.
   @param chain_len Number of hashes to create.
   @return A struct hash_chain filled with hashes.
 */
struct hash_chain hash_chain_create(void *base, int baselen, const EVP_MD *type,
                                    int chain_len)
{
  EVP_MD_CTX *ctx;
  struct hash_chain output;

  // Allocate space for our hash chain.
  output.digest_size = EVP_MD_size(type);
  output.chain_length = chain_len;
  output.data = calloc(output.chain_length, output.digest_size);

  // Hash the base data.
  ctx = EVP_MD_CTX_create();
  EVP_DigestInit_ex(ctx, type, NULL);
  EVP_DigestUpdate(ctx, base, baselen);
  EVP_DigestFinal_ex(ctx, output.data, NULL);

  // For each remaining item in the chain, hash the previous digest.
  for (int idx = 1; idx < output.chain_length; idx++) {
    EVP_DigestInit_ex(ctx, type, NULL);
    EVP_DigestUpdate(ctx, output.data + (idx - 1) * output.digest_size,
                     output.digest_size);
    EVP_DigestFinal_ex(ctx, output.data + idx * output.digest_size, NULL);
  }

  // Cleanup and return the chain.
  EVP_MD_CTX_destroy(ctx);
  return output;
}

/**
   @brief Verify that h comes directly before tip in a hash chain.
   @param h Pointer to a hash.
   @param tip Pointer to the "tip" hash.
   @param hash Hash algorithm to use.
   @param True if hash(h) == tip
 */
bool hash_chain_verify(const void *h, const void *tip, const EVP_MD *hash)
{
  EVP_MD_CTX *ctx;
  int result;
  int digest_len = EVP_MD_size(hash);
  void *data = malloc(digest_len);

  ctx = EVP_MD_CTX_create();
  EVP_DigestInit_ex(ctx, hash, NULL);
  EVP_DigestUpdate(ctx, h, digest_len);
  EVP_DigestFinal_ex(ctx, data, NULL);
  EVP_MD_CTX_destroy(ctx);

  result = memcmp(data, tip, digest_len);
  free(data);

  return result == 0;
}

/**
   @brief Print a hash chain into file f.
   @param chain The chain to print.
   @param f The file to write to.
 */
void hash_chain_print(struct hash_chain chain, FILE *f)
{
  BIO *out, *b64, *bio;
  b64 = BIO_new(BIO_f_base64());
  out = BIO_new_fp(f, BIO_NOCLOSE);
  bio = BIO_push(b64, out);

  for (int i = 0; i < chain.chain_length; i++) {
    BIO_write(bio, chain.data + i * chain.digest_size, chain.digest_size);
    BIO_flush(bio);
  }

  BIO_free_all(bio);
}

/**
   @brief Base64 decode a string. You must free the return value.
   @param str Some base64 encoded data.
   @param explen The expected length of the data you're reading.
   @returns Newly allocated pointer to buffer of length explen.
 */
void *base64_decode(char *str, int explen)
{
  uint8_t *buf = malloc(explen);
  BIO *b = BIO_new_mem_buf(str, -1);
  BIO *b64 = BIO_new(BIO_f_base64());
  BIO_push(b64, b);
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_read(b64, buf, explen);
  BIO_free_all(b64);
  return buf;
}

/**
   @brief Command for creating a hash chain.
 */
int cmd_create(int argc, char **argv)
{
  if (argc < 4) {
    fprintf(stderr, "error: too few args\n");
    fprintf(stderr, "usage: %s HASH LENGTH BASE\n", argv[0]);
    return EXIT_FAILURE;
  }

  const EVP_MD *hash = EVP_get_digestbyname(argv[1]);
  if (hash == NULL) {
    fprintf(stderr, "error: hash %s doesn't exist\n", argv[1]);
    return EXIT_FAILURE;
  }

  int length;
  if (sscanf(argv[2], "%d", &length) != 1) {
    fprintf(stderr, "error: can't convert %s to integer\n", argv[2]);
    return EXIT_FAILURE;
  }

  struct hash_chain chain = hash_chain_create(argv[3], strlen(argv[3]), hash, length);
  hash_chain_print(chain, stdout);
  free(chain.data);

  return EXIT_SUCCESS;
}

/**
   @brief Command for verifying a hash.
 */
int cmd_verify(int argc, char **argv)
{
  if (argc < 4) {
    fprintf(stderr, "error: too few args\n");
    fprintf(stderr, "usage: %s ALGO QUERY ANCHOR\n", argv[0]);
    return EXIT_FAILURE;
  }

  const EVP_MD *hash = EVP_get_digestbyname(argv[1]);
  if (hash == NULL) {
    fprintf(stderr, "error: hash %s doesn't exist\n", argv[1]);
    return EXIT_FAILURE;
  }

  int digest_len = EVP_MD_size(hash);
  void *qhash = base64_decode(argv[2], digest_len);
  void *thash = base64_decode(argv[3], digest_len);

  bool res = hash_chain_verify(qhash, thash, hash);
  free(qhash);
  free(thash);
  if (res) {
    printf("success\n");
    return EXIT_SUCCESS;
  } else {
    printf("failure\n");
    return EXIT_FAILURE;
  }
}

/**
   @brief Main entry point that calls the subcommands.
 */
int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "error: subcommand required\n");
    return EXIT_FAILURE;
  }

  OpenSSL_add_all_digests();
  int rv;

  if (strcmp(argv[1], "create") == 0) {
    rv = cmd_create(argc - 1, argv + 1);
  } else if (strcmp(argv[1], "verify") == 0) {
    rv = cmd_verify(argc - 1, argv + 1);
  } else {
    fprintf(stderr, "error: subcommand %s not found\n", argv[1]);
    rv = EXIT_FAILURE;
  }

  EVP_cleanup();
  return rv;
}
