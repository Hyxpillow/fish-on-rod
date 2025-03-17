
static unsigned char s_box[256];
static int i;
static int j;

void KSA(unsigned char* key, unsigned int key_length);
unsigned char PRGA();
void RC4_reset();