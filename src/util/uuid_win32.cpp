#include <rpc.h>
#include <stdio.h>

typedef unsigned char wuuid_t[16];

void uuid_clear(wuuid_t uu)
{
    memset(uu, 0, 16);
}

static void rpc_uuid_to_uuid_t(const UUID *uu, wuuid_t out)
{
    out[0] = (uu->Data1 >> 24) & 0xff;
    out[1] = (uu->Data1 >> 16) & 0xff;
    out[2] = (uu->Data1 >> 8) & 0xff;
    out[3] = (uu->Data1 >> 0) & 0xff;
    out[4] = (uu->Data2 >> 8) & 0xff;
    out[5] = (uu->Data2 >> 0) & 0xff;
    out[6] = (uu->Data3 >> 8) & 0xff;
    out[7] = (uu->Data3 >> 0) & 0xff;
    memcpy(out + 8, uu->Data4, 8);
}

static void uuid_t_to_rpc_uuid(const wuuid_t in, UUID *out)
{
    out->Data1 = (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3] << 0);
    out->Data2 = (in[4] << 8) | (in[5] << 0);
    out->Data3 = (in[6] << 8) | (in[7] << 0);
    memcpy(out->Data4, in + 8, 8);
}

void uuid_generate_random(wuuid_t out)
{
    UUID uu;
    UuidCreate(&uu);
    rpc_uuid_to_uuid_t(&uu, out);
}
void uuid_unparse(const wuuid_t uu, char *out)
{
    sprintf(out,
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%"
            "02x",
            uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7], uu[8], uu[9], uu[10], uu[11], uu[12], uu[13],
            uu[14], uu[15]);
}

int uuid_parse(const char *in, wuuid_t out)
{
    UUID uu;
    unsigned char c[37];
    memcpy(c, in, 37);
    RPC_STATUS ret = UuidFromString((unsigned char *)c, &uu);
    if (ret == RPC_S_OK) {
        rpc_uuid_to_uuid_t(&uu, out);
        return 0;
    }
    else {
        return -1;
    }
}

int uuid_compare(const wuuid_t uu1, const wuuid_t uu2)
{
    RPC_STATUS stat;
    UUID u1, u2;
    uuid_t_to_rpc_uuid(uu1, &u1);
    uuid_t_to_rpc_uuid(uu2, &u2);
    return UuidCompare(&u1, &u2, &stat);
}

int uuid_is_null(const wuuid_t uu)
{
    const unsigned char *cp;
    int i;

    for (i = 0, cp = uu; i < 16; i++)
        if (*cp++)
            return 0;
    return 1;
}
