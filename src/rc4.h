class RC4 {
private:
    unsigned char s_box[256];
    int i;
    int j;
public:
    void KSA(unsigned char *key, unsigned int key_length) {
        int r8 = 0;
        int r9 = 0;
        int tmp = 0;

        while (r8 < 256) {
            r9 = (r9 + s_box[r8] + key[r8 % key_length]) % 256;
            tmp = s_box[r8];
            s_box[r8] = s_box[r9];
            s_box[r9] = tmp;
            r8++;
        }
    }
    unsigned char PRGA() {
        int tmp_idx = 0;
        unsigned char tmp = 0;
    
        i = (i + 1) % 256;
        j = (j + s_box[i]) % 256;
        tmp = s_box[i];
        s_box[i] = s_box[j];
        s_box[j] = tmp;
    
        tmp_idx = (s_box[i] + s_box[j]) % 256;
        return s_box[tmp_idx];
    }
    void reset() {
        for (int idx = 0; idx < 256; idx++) {
            s_box[idx] = idx;
        }
        i = 0;
        j = 0;
    }
};

