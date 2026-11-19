#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include "beacon.h"

int main(int argc, char *argv[]) {
    const char *out_dir = (argc > 1) ? argv[1] : ".";

    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp224r1);
    if (!key || !EC_KEY_generate_key(key)) {
        fprintf(stderr, "Error: Failed to generate secp224r1 key\n");
        return 1;
    }

    const EC_POINT *pub = EC_KEY_get0_public_key(key);
    const BIGNUM *priv = EC_KEY_get0_private_key(key);
    const EC_GROUP *group = EC_KEY_get0_group(key);

    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    EC_POINT_get_affine_coordinates(group, pub, x, y, NULL);

    unsigned char pub_x[28];
    unsigned char priv_raw[28];
    memset(pub_x, 0, sizeof(pub_x));
    memset(priv_raw, 0, sizeof(priv_raw));

    BN_bn2binpad(x, pub_x, 28);
    BN_bn2binpad(priv, priv_raw, 28);

    char adv_b64[64];
    char priv_b64[64];
    findmy_base64_encode(pub_x, 28, adv_b64);
    findmy_base64_encode(priv_raw, 28, priv_b64);

    char adv_path[512];
    char priv_path[512];
    snprintf(adv_path, sizeof(adv_path), "%s/advertisement.key", out_dir);
    snprintf(priv_path, sizeof(priv_path), "%s/private.key", out_dir);

    FILE *fa = fopen(adv_path, "w");
    if (!fa) {
        perror("Failed to write advertisement.key");
        return 1;
    }
    fprintf(fa, "%s\n", adv_b64);
    fclose(fa);

    FILE *fp = fopen(priv_path, "w");
    if (!fp) {
        perror("Failed to write private.key");
        return 1;
    }
    fprintf(fp, "%s\n", priv_b64);
    fclose(fp);

    printf("=================================================================\n");
    printf(" findmy-linux Keypair Generated Successfully!\n");
    printf("=================================================================\n\n");
    printf("1. Advertisement Key (Saved to: %s):\n", adv_path);
    printf("   %s\n", adv_b64);
    printf("   (This is public. Your laptop broadcasts this key).\n\n");
    printf("2. Private Key (Saved to: %s):\n", priv_path);
    printf("   %s\n", priv_b64);
    printf("   (KEEP THIS SECRET! Use this on your phone/tracker to decrypt location).\n");
    printf("=================================================================\n");

    BN_free(x);
    BN_free(y);
    EC_KEY_free(key);
    return 0;
}
