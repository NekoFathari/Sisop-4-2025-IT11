#include "image_transfer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>

#define MAX_BUFFER 8192
#define INPUT_FOLDER ""
#define OUTPUT_FOLDER ""

void encode_hex(const char *input, char *output) {
    FILE *fp = fopen(input, "r");
    if (!fp) {
        perror("Gagal membuka file");
        strcpy(output, "ERROR");
        return;
    }

    char ch;
    int i = 0;
    while ((ch = fgetc(fp)) != EOF) {
        sprintf(&output[i], "%02x", (unsigned char)ch);
        i += 2;
    }
    output[i] = '\0';
    fclose(fp);
}

void save_from_hex(const char *filename, const char *hex) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Gagal menyimpan file");
        return;
    }

    for (int i = 0; hex[i] && hex[i+1]; i += 2) {
        char byte[3] = {hex[i], hex[i+1], '\0'};
        unsigned char val = (unsigned char)strtol(byte, NULL, 16);
        fwrite(&val, 1, 1, fp);
    }

    fclose(fp);
}

int main() {
    CLIENT *clnt;
    clnt = clnt_create("localhost", IMAGE_TRANSFER_PROG, IMAGE_TRANSFER_VERS, "udp");
    if (!clnt) {
        clnt_pcreateerror("localhost");
        exit(1);
    }

    int choice;
    char input_name[128], filepath[256], encoded[MAX_BUFFER], *result;

    while (1) {
        printf("\n=== Image Decoder Client ===\n");
        printf("1. Send input file to server\n");
        printf("2. Download file from server\n");
        printf("3. Exit\n>> ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            printf("Enter file name (e.g. input_1.txt): ");
            fgets(input_name, sizeof(input_name), stdin);
            input_name[strcspn(input_name, "\n")] = 0;

            snprintf(filepath, sizeof(filepath), "%s%s", INPUT_FOLDER, input_name);
            encode_hex(filepath, encoded);
            if (strcmp(encoded, "ERROR") == 0) continue;

            result = *decrypt_1(&encoded, clnt);
            printf("Decrypted and saved as: %s.jpeg\n", result);

        } else if (choice == 2) {
            printf("Enter JPEG file name (e.g. 1744xxxx.jpeg): ");
            fgets(input_name, sizeof(input_name), stdin);
            input_name[strcspn(input_name, "\n")] = 0;

            result = *download_1(&input_name, clnt);
            if (strncmp(result, "ERROR", 5) == 0)
                printf("Server: %s\n", result);
            else {
                snprintf(filepath, sizeof(filepath), "%s%s", OUTPUT_FOLDER, input_name);
                save_from_hex(filepath, result);
                printf("Downloaded to: %s\n", filepath);
            }

        } else if (choice == 3) {
            printf("Exiting...\n");
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }

    clnt_destroy(clnt);
    return 0;
}
