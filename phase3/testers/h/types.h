#ifndef TYPES
#define TYPES

typedef struct ssi_payload_t
{
    int service_code;
    void *arg;
} ssi_payload_t, *ssi_payload_PTR;

typedef struct sst_print_t
{
    int length;
    char *string;
} sst_print_t, *sst_print_PTR;

#endif
