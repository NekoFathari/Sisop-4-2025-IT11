program IMAGE_TRANSFER_PROG {
    version IMAGE_TRANSFER_VERS {
        string DECRYPT(string) = 1;
        string DOWNLOAD(string) = 2;
    } = 1;
} = 0x31234567;
