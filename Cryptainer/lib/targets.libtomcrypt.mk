LIBTOMCRYPT.SOURCE=\
	libtomcrypt/cfb_getiv.c \
	libtomcrypt/crypt_prng_is_valid.c \
	libtomcrypt/dh.c \
	libtomcrypt/yarrow.c \
	libtomcrypt/eax_init.c \
	libtomcrypt/hmac_process.c \
	libtomcrypt/rmd128.c \
	libtomcrypt/rmd160.c \
	libtomcrypt/ocb_ntz.c \
	libtomcrypt/pkcs_1_v15_sa_decode.c \
	libtomcrypt/ofb_encrypt.c \
	libtomcrypt/crypt_unregister_cipher.c \
	libtomcrypt/cfb_encrypt.c \
	libtomcrypt/rsa_sign_hash.c \
	libtomcrypt/dsa_verify_key.c \
	libtomcrypt/pmac_shift_xor.c \
	libtomcrypt/dsa_sign_hash.c \
	libtomcrypt/eax_encrypt_authenticate_memory.c \
	libtomcrypt/crypt.c \
	libtomcrypt/eax_encrypt.c \
	libtomcrypt/packet_store_header.c \
	libtomcrypt/zeromem.c \
	libtomcrypt/aes.c \
	libtomcrypt/pmac_test.c \
	libtomcrypt/ofb_decrypt.c \
	libtomcrypt/des.c \
	libtomcrypt/ecc.c \
	libtomcrypt/eax_decrypt_verify_memory.c \
	libtomcrypt/cfb_decrypt.c \
	libtomcrypt/md2.c \
	libtomcrypt/md4.c \
	libtomcrypt/md5.c \
	libtomcrypt/mpi.c \
	libtomcrypt/rc2.c \
	libtomcrypt/rc4.c \
	libtomcrypt/rc5.c \
	libtomcrypt/rc6.c \
	libtomcrypt/eax_decrypt.c \
	libtomcrypt/eax_test.c \
	libtomcrypt/pkcs_1_oaep_encode.c \
	libtomcrypt/hmac_done.c \
	libtomcrypt/rsa_v15_verify_hash.c \
	libtomcrypt/safer.c \
	libtomcrypt/rsa_verify_hash.c \
	libtomcrypt/hmac_file.c \
	libtomcrypt/crypt_find_hash.c \
	libtomcrypt/rand_prime.c \
	libtomcrypt/crypt_unregister_hash.c \
	libtomcrypt/omac_process.c \
	libtomcrypt/sober128.c \
	libtomcrypt/noekeon.c \
	libtomcrypt/dsa_free.c \
	libtomcrypt/hmac_init.c \
	libtomcrypt/ecb_encrypt.c \
	libtomcrypt/pkcs_1_oaep_decode.c \
	libtomcrypt/crypt_cipher_is_valid.c \
	libtomcrypt/pmac_ntz.c \
	libtomcrypt/ecb_decrypt.c \
	libtomcrypt/crypt_find_hash_id.c \
	libtomcrypt/crypt_find_cipher_id.c \
	libtomcrypt/omac_memory.c \
	libtomcrypt/crypt_find_prng.c \
	libtomcrypt/crypt_cipher_descriptor.c \
	libtomcrypt/pkcs_1_v15_es_encode.c \
	libtomcrypt/crypt_find_hash_any.c \
	libtomcrypt/crypt_unregister_prng.c \
	libtomcrypt/rsa_v15_sign_hash.c \
	libtomcrypt/omac_done.c \
	libtomcrypt/cbc_setiv.c \
	libtomcrypt/omac_file.c \
	libtomcrypt/crypt_hash_descriptor.c \
	libtomcrypt/cbc_encrypt.c \
	libtomcrypt/hmac_test.c \
	libtomcrypt/crypt_find_cipher_any.c \
	libtomcrypt/base64_encode.c \
	libtomcrypt/sprng.c \
	libtomcrypt/omac_init.c \
	libtomcrypt/ocb_init.c \
	libtomcrypt/pkcs_1_v15_es_decode.c \
	libtomcrypt/cbc_decrypt.c \
	libtomcrypt/tiger.c \
	libtomcrypt/pmac_memory.c \
	libtomcrypt/rng_get_bytes.c \
	libtomcrypt/base64_decode.c \
	libtomcrypt/blowfish.c \
	libtomcrypt/cbc_start.c \
	libtomcrypt/ctr_setiv.c \
	libtomcrypt/hash_file.c \
	libtomcrypt/ocb_shift_xor.c \
	libtomcrypt/crypt_find_cipher.c \
	libtomcrypt/mpi_to_ltc_error.c \
	libtomcrypt/omac_test.c \
	libtomcrypt/ocb_test.c \
	libtomcrypt/pkcs_1_os2ip.c \
	libtomcrypt/hmac_memory.c \
	libtomcrypt/cbc_getiv.c \
	libtomcrypt/hash_memory.c \
	libtomcrypt/pkcs_5_1.c \
	libtomcrypt/pkcs_5_2.c \
	libtomcrypt/ofb_setiv.c \
	libtomcrypt/packet_valid_header.c \
	libtomcrypt/pkcs_1_pss_encode.c \
	libtomcrypt/tim_exptmod.c \
	libtomcrypt/sha1.c \
	libtomcrypt/ecb_start.c \
	libtomcrypt/rsa_v15_decrypt_key.c \
	libtomcrypt/rsa_decrypt_key.c \
	libtomcrypt/ctr_start.c \
	libtomcrypt/skipjack.c \
	libtomcrypt/crypt_prng_descriptor.c \
	libtomcrypt/hash_filehandle.c \
	libtomcrypt/fortuna.c \
	libtomcrypt/dsa_import.c \
	libtomcrypt/ocb_encrypt.c \
	libtomcrypt/pkcs_1_pss_decode.c \
	libtomcrypt/xtea.c \
	libtomcrypt/pkcs_1_mgf1.c \
	libtomcrypt/ocb_encrypt_authenticate_memory.c \
	libtomcrypt/burn_stack.c \
	libtomcrypt/pmac_process.c \
	libtomcrypt/ctr_getiv.c \
	libtomcrypt/cfb_setiv.c \
	libtomcrypt/rsa_import.c \
	libtomcrypt/ctr_encrypt.c \
	libtomcrypt/ofb_start.c \
	libtomcrypt/rsa_free.c \
	libtomcrypt/ocb_decrypt.c \
	libtomcrypt/dsa_make_key.c \
	libtomcrypt/ocb_decrypt_verify_memory.c \
	libtomcrypt/is_prime.c \
	libtomcrypt/twofish_tab.c \
	libtomcrypt/crypt_hash_is_valid.c \
	libtomcrypt/rng_make_prng.c \
	libtomcrypt/crypt_register_cipher.c \
	libtomcrypt/ctr_decrypt.c \
	libtomcrypt/crypt_argchk.c \
	libtomcrypt/eax_addheader.c \
	libtomcrypt/whirl.c \
	libtomcrypt/ofb_getiv.c \
	libtomcrypt/cast5.c \
	libtomcrypt/sha256.c \
	libtomcrypt/crypt_register_hash.c \
	libtomcrypt/rsa_exptmod.c \
	libtomcrypt/sha512.c \
	libtomcrypt/s_ocb_done.c \
	libtomcrypt/dsa_verify_hash.c \
	libtomcrypt/safer_tab.c \
	libtomcrypt/pkcs_1_i2osp.c \
	libtomcrypt/ocb_done_encrypt.c \
	libtomcrypt/pmac_done.c \
	libtomcrypt/cfb_start.c \
	libtomcrypt/rsa_v15_encrypt_key.c \
	libtomcrypt/dsa_export.c \
	libtomcrypt/pmac_file.c \
	libtomcrypt/rsa_encrypt_key.c \
	libtomcrypt/pkcs_1_v15_sa_encode.c \
	libtomcrypt/eax_done.c \
	libtomcrypt/rsa_make_key.c \
	libtomcrypt/ocb_done_decrypt.c \
	libtomcrypt/rsa_export.c \
	libtomcrypt/error_to_string.c \
	libtomcrypt/saferp.c \
	libtomcrypt/pmac_init.c \
	libtomcrypt/twofish.c \
	libtomcrypt/crypt_register_prng.c

LIBTOMCRYPT.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(LIBTOMCRYPT.SOURCE:.c=$O))
SOURCE.ALL += $(LIBTOMCRYPT.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.libtomcrypt.mk

$(DIR.OBJ)/libtomcrypt$(EXT.LIB): CFLAGS+=-Ilibtomcrypt

ifeq "$(OS)" "WIN32"
$(DIR.OBJ)/libtomcrypt$(EXT.LIB): CFLAGS+=-DWIN32 
else
$(DIR.OBJ)/libtomcrypt$(EXT.LIB): CFLAGS+=-DDEVRANDOM 
endif

ifneq "$(COMPILER)" "VC6"
# GCC Options
$(DIR.OBJ)/libtomcrypt$(EXT.LIB): CFLAGS+=-Wall -Wsign-compare -W -Wshadow

# Optimize if release
ifeq "$(BUILD)" "RELEASE"
$(DIR.OBJ)/libtomcrypt$(EXT.LIB): CFLAGS+=-O3 -funroll-loops -fomit-frame-pointer 
endif

endif

$(DIR.OBJ)/libtomcrypt$(EXT.LIB): $(DIR.LIST) $(LIBTOMCRYPT.SOURCE.OBJ)
	$(DO.LINK.LIB)

libtomcryptclean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif
