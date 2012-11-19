/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines cryptographic functions including secure key exchange
/// based on the curve25519 paper (see http://cr.yp.to/ecdh.html) as well as
/// SHA hashing and AES symmetric encryption. The idea is to use the key
/// exchange to establish a shared secret, and then use symmetric encryption
/// for secure communications. The curve25519 implementation was chosen because
/// it is fast (so you can change secrets frequently) and the implementation is
/// relatively concise (under 1000 lines of C code.) Cryptographically-secure
/// random data is obtained by reading from /dev/random or /dev/urandom on
/// UNIX-like systems, and by using RtlGenRandom on Windows.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBCRYPTO_HPP_INCLUDED
#define LIBCRYPTO_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace crypto {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// The state structure for the Blowfish encryption algorithm.
/// See http://www.schneier.com/blowfish.html
struct blowfish_context_t
{
    uint32_t P[18];     /// P-box
    uint32_t S[4][256]; /// S-boxes
};

/// Generates a secret key suitable for use with curve25519, using the method
/// described at http://cr.yp.to/ecdh.html
/// This function may block the calling thread until sufficient entropy is
/// available.
///
/// @param out_key Pointer to a 32-byte buffer that will be filled with the
/// secret key.
CMN_PUBLIC void secret_key(uint8_t *out_key);

/// Generates a public key for a given secret key, using the method described
/// at http://cr.yp.to/ecdh.html
///
/// @param secret Pointer to a 32-byte buffer containing the secret key
/// previously computed using the secret_key() function.
/// @param out_public Pointer to a 32-byte buffer that will be filled with the
/// public key that corresponds to @a secret.
CMN_PUBLIC void public_key(uint8_t const *secret, uint8_t *out_public);

/// Generates a shared secret that can be used to encrypt communications with
/// a given user, using the method described at http://cr.yp.to/ecdh.html
///
/// @param our_secret Pointer to a 32-byte buffer containing the secret key
/// previously generated using the secret_key() function.
/// @param her_public Pointer to a 32-byte buffer containing the public key
/// of another user, previously generated using her equivalent of the function
/// public_key().
/// @param out_shared Pointer to a 32-byte buffer that will be filled with the
/// shared secret computed using the @a our_secret and @a her_public keys.
CMN_PUBLIC void shared_secret(
    uint8_t const *our_secret,
    uint8_t const *her_public,
    uint8_t       *out_shared);

/// Implements the raw curve25519 function described in Dan Bernstein's paper.
/// Usage is the same as described at http://cr.yp.to/ecdh.html
/// This function is typically not used directly; instead, use the functions
/// public_key() and shared_secret(), which call this function internally. The
/// API of this function maintains compatibility with the Bernstein
/// implementation.
///
/// @param out_buf Pointer to a 32-byte buffer that will be filled with the
/// result of the function.
/// @param secret Pointer to a 32-byte buffer containing a secret key
/// previously computed using the secret_key() function.
/// @param basepoint Pointer to a 32-byte buffer whose contents depends on the
/// context in which the function is being called. See the original description
/// at http://cr.yp.to/ecdh.html for more information.
CMN_PUBLIC void curve25519(
    uint8_t       *out_buf,
    uint8_t const *secret,
    uint8_t const *basepoint);

/// Initializes a Blowfish encryption or decryption context. After calling this
/// function, use blowfish_encrypt() or blowfish_decrypt() to perform the
/// desired operation.
///
/// @param key Pointer to a byte array containing the private key.
/// @param key_size The size of the byte array @a key specified in bytes.
/// @param context The Blowfish context structure to initialize.
CMN_PUBLIC void blowfish_context_init(
    uint8_t const              *key,
    size_t                      key_size,
    crypto::blowfish_context_t *context);

CMN_PUBLIC void blowfish_encrypt(
    uint64_t                    plain_text,
    uint64_t                   *out_cypher_text,
    crypto::blowfish_context_t *context);

CMN_PUBLIC void blowfish_decrypt(
    uint32_t const             *xl,
    uint32_t                   *xr,
    crypto::blowfish_context_t *context);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace crypto */

#endif /* LIBCRYPTO_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
