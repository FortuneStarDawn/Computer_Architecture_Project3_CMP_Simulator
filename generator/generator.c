#include <stdio.h>
int encode(unsigned int buff);
//78563412
int main()
{
    int buff;
    FILE *iimage, *dimage;
    iimage = fopen("iimage.bin", "wb");
    dimage = fopen("dimage.bin", "wb");

    buff = encode(0x00000000);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x0000000B);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000000);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000010);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000000);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000020);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000000);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000040);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000080);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C0000C0);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000100);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0x8C000000);
    fwrite(&buff, sizeof(int), 1, iimage);
    buff = encode(0xFFFFFFFF);
    fwrite(&buff, sizeof(int), 1, iimage);

    buff = encode(0x00000000);
    fwrite(&buff, sizeof(int), 1, dimage);
    buff = encode(0x00000001);
    fwrite(&buff, sizeof(int), 1, dimage);
    buff = encode(0xFFFFFFFF);
    fwrite(&buff, sizeof(int), 1, dimage);

    return 0;
}

int encode(unsigned int origin)
{
    int answer=0;
    answer |= origin>>24;
    answer |= origin>>8&0xFF00;
    answer |= origin<<8&0xFF0000;
    answer |= origin<<24;
    return answer;
}
