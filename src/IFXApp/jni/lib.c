/**
 * MIT License
 *
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 */

#include "lib.h"

#include <string.h>
#include <tss2/tss2_rc.h>
#include <tss2/tss2_mu.h>
#include <tss2/tss2_sys.h>
#include <tss2/tss2_esys.h>
#include <tss2/tss2_tctildr.h>

#include <openssl/err.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

void Java_com_ifx_nave_JavaNative_helloWorld(JNIEnv* env __unused, jobject obj __unused)
{
    (void) env;
    ALOGI("Invoked Java_com_ifx_nave_JavaNative_helloWorld");
    ALOGI("Exit Java_com_ifx_nave_JavaNative_helloWorld");
}

void Java_com_ifx_nave_JavaNative_tss2Examples(JNIEnv* env __unused, jobject obj __unused)
{
    (void) env;
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *tcti = NULL;
    ESYS_CONTEXT *esys_ctx = NULL;
    size_t i;
    char string[33];

    /* Modify this to utilize a different TPM target, such as a real TPM: "device:/dev/tpmrm0". */
    const char *tcti_name_conf = "mssim:host=localhost,port=2321";

    TPM2B_DIGEST *random_bytes = NULL;

    ALOGI("Invoked Java_com_ifx_nave_JavaNative_tss2Examples");

    rc = Tss2_TctiLdr_Initialize(tcti_name_conf, &tcti);
    if (rc != TSS2_RC_SUCCESS) {
        ALOGE("Tss2_TctiLdr_Initialize has failed.");
        goto out;
    }

    rc = Esys_Initialize(&esys_ctx, tcti, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        goto out_tcti_finalize;
    }

    rc = Esys_Startup(esys_ctx, TPM2_SU_CLEAR);
    if (rc != TPM2_RC_SUCCESS && rc != TPM2_RC_INITIALIZE) {
        ALOGE("Esys_Startup has failed with error code: 0x%" PRIX32 "(%s).\n",
              rc, Tss2_RC_Decode(rc));
        goto out_esys_finalize;
    }

    rc = Esys_GetRandom(esys_ctx, ESYS_TR_NONE,
                        ESYS_TR_NONE, ESYS_TR_NONE,
                        TPM2_SHA256_DIGEST_SIZE, &random_bytes);
    if (rc != TPM2_RC_SUCCESS) {
        ALOGE("Esys_GetRandom has failed with error code: 0x%" PRIX32 "(%s).\n",
              rc, Tss2_RC_Decode(rc));
        goto out_esys_finalize;
    }

    if (random_bytes->size != TPM2_SHA256_DIGEST_SIZE) {
        ALOGE("Esys_GetRandom returns an incorrect size.");
        goto out_free_random_bytes;
    }

    for (i = 0; i < random_bytes->size; i++) {
        snprintf(string + strlen(string), sizeof(string) - strlen(string),
                 "%02x", random_bytes->buffer[i]);
    }
    ALOGI("Esys_GetRandom (%dB): 0x%s", random_bytes->size, string);

out_free_random_bytes:
    free(random_bytes);
out_esys_finalize:
    Esys_Finalize(&esys_ctx);
out_tcti_finalize:
    Tss2_TctiLdr_Finalize(&tcti);
out:
    ALOGI("Exit Java_com_ifx_nave_JavaNative_tss2Examples");
}

void Java_com_ifx_nave_JavaNative_ossl3Examples(JNIEnv* env __unused, jobject obj __unused)
{
    (void) env;
    OSSL_PROVIDER *prov_default = NULL;
    OSSL_PROVIDER *prov_tpm2 = NULL;
    unsigned char buf[32];
    size_t i;
    char string[33];
    int rc;

    /* Modify this to utilize a different TPM target, such as a real TPM: "device:/dev/tpmrm0". */
    const char *tcti_name_conf = "mssim:host=localhost,port=2321";

    ALOGI("Invoked Java_com_ifx_nave_JavaNative_ossl3Examples");

    /* Load default provider */
    if ((prov_default = OSSL_PROVIDER_load(NULL, "default")) == NULL) {
        ALOGE("OSSL_PROVIDER_load(\"default\") has failed.");
        goto out;
    }

    /* Self-test */
    if (!OSSL_PROVIDER_self_test(prov_default)) {
        ALOGE("OSSL_PROVIDER_self_test(\"default\") has failed.");
        goto out_provider_unload_default;
    }

    /* Configure the TCTI */
    /* For OpenSSL >= 3.2, use the following: */
    /*
        OSSL_PARAM params[2];
        params[0] = OSSL_PARAM_construct_utf8_ptr("tcti", &tcti_name_conf, 0);
        params[1] = OSSL_PARAM_construct_end();
        OSSL_PROVIDER_load_ex(NULL, "/system/lib64/libtss2-ossl3-provider.so", params))
    */
    if (setenv("TPM2OPENSSL_TCTI", tcti_name_conf, 1) != 0) {
        ALOGE("Unable to configure TCTI: %s.", tcti_name_conf);
        goto out_provider_unload_default;
    }

    /* Load TPM2 provider */
    if ((prov_tpm2 = OSSL_PROVIDER_load(NULL, "/system/lib64/libtss2-ossl3-provider.so")) == NULL) {
        unsigned long err = ERR_get_error();
        ALOGE("OSSL_PROVIDER_load(\"libtss2-ossl3-provider.so\") has failed.");
        ALOGE("Error loading provider: %lu - %s\n", err, ERR_error_string(err, NULL));
        goto out_provider_unload_default;
    }

    /* Self-test */
    if (!OSSL_PROVIDER_self_test(prov_tpm2)) {
        ALOGE("OSSL_PROVIDER_self_test(\"libtss2-ossl3-provider\") has failed.");
        goto out_provider_unload_tpm2;
    }

    /* Get random */
    memset(buf, 0, sizeof(buf));
    rc = RAND_bytes(buf, sizeof(buf));
    if (rc != 1) {
        ALOGE("RAND_bytes has failed.");
    } else {
        for (i = 0; i < sizeof(buf); i++) {
            snprintf(string + strlen(string), sizeof(string) - strlen(string), "%02x", buf[i]);
        }
        ALOGI("RAND_bytes (%ldB): 0x%s", sizeof(buf), string);
    }

out_provider_unload_tpm2:
    OSSL_PROVIDER_unload(prov_tpm2);
out_provider_unload_default:
    OSSL_PROVIDER_unload(prov_default);
out:
    ALOGI("Exit Java_com_ifx_nave_JavaNative_ossl3Examples");
}
