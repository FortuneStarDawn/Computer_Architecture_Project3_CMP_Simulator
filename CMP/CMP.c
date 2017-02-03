#include <stdio.h>
#include <stdlib.h>

int decode(unsigned int buff);

int main(int argc, char *argv[])
{
	FILE *iImage, *dImage, *result, *report;
	int iDisk[256]={0}, dDisk[256]={0}, reg[32]={0}, PC=0, iTotal=0, dTotal=0, cycle=0, opcode, funct, rs, rt, rd, shamt, imm, immu, immj, addr, i, j;
    unsigned int buff, IChit=0, ICmiss=0, DChit=0, DCmiss=0, IThit=0, ITmiss=0, DThit=0, DTmiss=0, IPhit=0, IPmiss=0, DPhit=0, DPmiss=0;
    int *IC, *DC, *IMRU, *DMRU, *IT, temp, temp2, *DT, ITsize, DTsize, IPsize, DPsize, ICsize, DCsize, ICmod, DCmod, ICset, DCset, Iway, Dway, *ITLRU, *DTLRU;
    int ini[11], *IM, *DM, IMsize, DMsize, IPoffest, DPoffest, *IMLRU, *DMLRU, *IP, *DP, page, *ICvalid, *DCvalid, start, end, ICoffest, DCoffest, PPN, PA, set, tag;

    if(argc==11)
    {
        for(i=1; i<11; i++) ini[i] = atoi(argv[i]);
        IMsize = ini[1]/ini[3];
        DMsize = ini[2]/ini[4];
        IPsize = 1024/ini[3];
        DPsize = 1024/ini[4];
        IPoffest = ini[3];
        DPoffest = ini[4];
        ITsize = IPsize/4;
        DTsize = DPsize/4;
        ICsize = ini[5]/ini[6];
        DCsize = ini[8]/ini[9];
        ICoffest = ini[6];
        DCoffest = ini[9];
        ICmod = ICsize/ini[7];
        DCmod = DCsize/ini[10];
        Iway = ini[7];
        Dway = ini[10];
    }
    else //default
    {
        IMsize = 8;
        DMsize = 2;
        IPsize = 128;
        DPsize = 64;
        IPoffest = 8;
        DPoffest = 16;
        ITsize = 32;
        DTsize = 16;
        ICsize = 4;
        DCsize = 4;
        ICoffest = 4;
        DCoffest = 4;
        ICmod = 1;
        DCmod = 4;
        Iway = 4;
        Dway = 1;
    }

    ICset = ICsize/ICmod;
    DCset = DCsize/DCmod;

    IM = (int *)malloc(IMsize*sizeof(int));
    DM = (int *)malloc(DMsize*sizeof(int));
    IMLRU = (int *)malloc(IMsize*sizeof(int));
    DMLRU = (int *)malloc(DMsize*sizeof(int));

    IT = (int *)malloc(ITsize*sizeof(int));
    DT = (int *)malloc(DTsize*sizeof(int));
    ITLRU = (int *)malloc(ITsize*sizeof(int));
    DTLRU = (int *)malloc(DTsize*sizeof(int));

    IC = (int *)malloc(ICsize*sizeof(int));
    DC = (int *)malloc(DCsize*sizeof(int));
    IMRU = (int *)malloc(ICsize*sizeof(int));
    DMRU = (int *)malloc(DCsize*sizeof(int));
    ICvalid = (int *)malloc(ICsize*sizeof(int));
    DCvalid = (int *)malloc(DCsize*sizeof(int));

    IP = (int *)malloc(IPsize*sizeof(int));
    DP = (int *)malloc(DPsize*sizeof(int));

    for(i=0; i<ICsize; i++)
    {
        IC[i]=-1;
        IMRU[i]=0;
        ICvalid[i]=0;
    }
    for(i=0; i<DCsize; i++)
    {
        DC[i]=-1;
        DMRU[i]=0;
        DCvalid[i]=0;
    }
    for(i=0; i<ITsize; i++)
    {
        IT[i]=-1;
        ITLRU[i]=0;
    }
    for(i=0; i<DTsize; i++)
    {
        DT[i]=-1;
        DTLRU[i]=0;
    }
    for(i=0; i<IMsize; i++)
    {
        IM[i]=-1;
        IMLRU[i]=0;
    }
    for(i=0; i<DMsize; i++)
    {
        DM[i]=-1;
        DMLRU[i]=0;
    }
    for(i=0; i<IPsize; i++) IP[i]=0;
    for(i=0; i<DPsize; i++) DP[i]=0;

    iImage = fopen("iimage.bin", "rb");
    dImage = fopen("dimage.bin", "rb");
    result = fopen("snapshot.rpt", "w");
    report = fopen("report.rpt", "w");
    if(fread(&buff, sizeof(int), 1, iImage)) PC = decode(buff)/4;
    if(fread(&buff, sizeof(int), 1, dImage)) reg[29] = decode(buff);
    if(fread(&buff, sizeof(int), 1, iImage)) iTotal = decode(buff);
    if(fread(&buff, sizeof(int), 1, dImage)) dTotal = decode(buff);
    for(i=PC; i<PC+iTotal; i++) if(fread(&buff, sizeof(int), 1, iImage)) iDisk[i] = decode(buff);
    for(i=0; i<dTotal; i++) if(fread(&buff, sizeof(int), 1, dImage)) dDisk[i] = decode(buff);
    fclose(iImage);
    fclose(dImage);

    while(1)
    {
        fprintf(result, "cycle %d\n", cycle);
        for(i=0; i<32; i++) fprintf(result, "$%02d: 0x%08X\n", i, reg[i]);
        fprintf(result, "PC: 0x%08X\n\n\n", PC*4);

        page = PC*4/IPoffest;
        for(i=0; i<ITsize; i++) ITLRU[i]++;
        for(i=0; i<IMsize; i++) IMLRU[i]++;

        for(i=0; i<ITsize; i++)
        {
            if(IT[i]==page)
            {
                IThit++;
                ITLRU[i]=0;
                for(j=0; j<IMsize; j++)
                {
                    if(IM[j]==page)
                    {
                        PPN = j;
                        break;
                    }
                }
                break;
            }
        }

        if(i==ITsize)
        {
            ITmiss++;
            if(IP[page])
            {
                IPhit++;
                for(i=0; i<IMsize; i++)
                {
                    if(IM[i]==page)
                    {
                        IMLRU[i]=0;
                        PPN = i;
                        break;
                    }
                }
            }
            else
            {
                IPmiss++;
                IP[page]=1;

                temp=0;
                for(i=0; i<IMsize; i++)
                {
                    if(IMLRU[i]>temp)
                    {
                        temp=IMLRU[i];
                        temp2=i;
                    }
                }
                temp = IM[temp2];
                IM[temp2]=page;
                IMLRU[temp2]=0;
                PPN = temp2;

                for(i=0; i<ICsize; i++)
                {
                    if((IC[i]*ICoffest*ICmod/IPoffest)==PPN)
                    {
                        IC[i]=-1;
                        IMRU[i]=0;
                        ICvalid[i]=0;
                    }
                }
                for(i=0; i<ITsize; i++)
                {
                    if(IT[i]==temp)
                    {
                        IT[i]=-1;
                        ITLRU[i]=1000000;
                    }
                }
                if(IP[temp]) IP[temp]=0;
            }

            temp=0;
            for(i=0; i<ITsize; i++)
            {
                if(ITLRU[i]>temp)
                {
                    temp=ITLRU[i];
                    temp2=i;
                }
            }
            IT[temp2]=page;
            ITLRU[temp2]=0;
        }

        PA = PPN*IPoffest + PC*4%IPoffest;
        set = PA/ICoffest%ICmod;
        tag = PA/ICoffest/ICmod;
        start = set*ICset;
        end = (set+1)*ICset;

        for(i=start; i<end; i++)
        {
            if(IC[i]==tag)
            {
                IChit++;
                for(j=start; j<end; j++) if(j!=i && IMRU[j]==0) break;
                if(j==end) for(j=start; j<end; j++) IMRU[j]=0;
                if(Iway!=1) IMRU[i]=1;
                break;
            }
        }
        if(i==end)
        {
            ICmiss++;
            for(i=start; i<end; i++)
            {
                if(ICvalid[i]==0)
                {
                    IC[i]=tag;
                    for(j=start; j<end; j++) if(j!=i && IMRU[j]==0) break;
                    if(j==end) for(j=start; j<end; j++) IMRU[j]=0;
                    if(Iway!=1) IMRU[i]=1;
                    ICvalid[i]=1;
                    break;
                }
            }

            if(i==end)
            {
                for(i=start; i<end; i++)
                {
                    if(IMRU[i]==0)
                    {
                        IC[i]=tag;
                        for(j=start; j<end; j++) if(j!=i && IMRU[j]==0) break;
                        if(j==end) for(j=start; j<end; j++) IMRU[j]=0;
                        if(Iway!=1) IMRU[i]=1;
                        break;
                    }
                }
            }
        }

        opcode = (unsigned int)iDisk[PC]>>26;
        rs = iDisk[PC]>>21&0x1F;
        rt = iDisk[PC]>>16&0x1F;
        rd = iDisk[PC]>>11&0x1F;
        shamt = iDisk[PC]>>6&0x1F;
        funct = iDisk[PC]&0x3F;
        imm = iDisk[PC]<<16>>16;
        immu = iDisk[PC]&0xFFFF;
        immj = iDisk[PC]&0x03FFFFFF;
        addr = reg[rs] + imm;
        i = addr/4;
        j = addr%4;
        PC++;
        cycle++;

        if(opcode == 0x00) //R type
        {
            if(funct==0x08) PC = reg[rs]/4; //jr
            else if(rd!=0) //sll, srl, sra, sub, and, or, xor, nor, nand, slt, add, addu
            {
                if(funct==0x00) reg[rd] = reg[rt] << shamt; //sll
                else if(funct==0x02) reg[rd] = (unsigned int)reg[rt] >> shamt; //srl
                else if(funct==0x03) reg[rd] = reg[rt] >> shamt; //sra
                else if(funct==0x22) reg[rd] = reg[rs] - reg[rt]; //sub
                else if(funct==0x24) reg[rd] = reg[rs] & reg[rt]; //and
                else if(funct==0x25) reg[rd] = reg[rs] | reg[rt]; //or
                else if(funct==0x26) reg[rd] = reg[rs] ^ reg[rt]; //xor
                else if(funct==0x27) reg[rd] = ~(reg[rs] | reg[rt]); //nor
                else if(funct==0x28) reg[rd] = ~(reg[rs] & reg[rt]); //nand
                else if(funct==0x2A) reg[rd] = reg[rs] < reg[rt]; //slt
                else reg[rd] = reg[rs] + reg[rt]; //add, addu
            }
        }
        else // I J S type
        {
            if(opcode<8) //j, jal, beq, bne, bgtz
            {
                if(opcode==0x02) PC = immj; //j
                else if(opcode==0x03) //jal
                {
                    reg[31] = PC*4;
                    PC = immj;
                }
                else if(opcode==0x04) //beq
                {
                    if(reg[rs]==reg[rt]) PC = PC + imm;
                }
                else if(opcode==0x05) //bne
                {
                    if(reg[rs]!=reg[rt]) PC = PC + imm;
                }
                else //bgtz
                {
                    if(reg[rs]>0) PC = PC + imm;
                }
            }
            else if(opcode<32) //slti, andi, ori, nori, lui, addi, addiu
            {
                if(rt!=0)
                {
                    if(opcode==0x0A) reg[rt] = reg[rs] < imm; //slti
                    else if(opcode==0x0C) reg[rt] = reg[rs] & immu; //andi
                    else if(opcode==0x0D) reg[rt] = reg[rs] | immu; //ori
                    else if(opcode==0x0E) reg[rt] = ~(reg[rs] | immu); //nori
                    else if(opcode==0x0F) reg[rt] = imm << 16; //lui
                    else reg[rt] = reg[rs] + imm; //addi, addiu
                }
            }
            else if(opcode<50) //lb, lh, lw, lbu, lhu, sb, sh, sw
            {
                if(opcode==0x20) //lb
                {
                    if(rt!=0)
                    {
                        if(j==3) reg[rt] = dDisk[i]<<24>>24;
                        else if(j==2) reg[rt] = dDisk[i]<<16>>24;
                        else if(j==1) reg[rt] = dDisk[i]<<8>>24;
                        else reg[rt] = dDisk[i]>>24;
                    }
                }
                else if(opcode==0x21) //lh
                {
                    if(rt!=0)
                    {
                        if(j==2) reg[rt] = dDisk[i]<<16>>16;
                        else reg[rt] = dDisk[i]>>16;
                    }
                }
                else if(opcode==0x23) //lw
                {
                    if(rt!=0) reg[rt] = dDisk[i];
                }
                else if(opcode==0x24) //lbu
                {
                    if(rt!=0)
                    {
                        if(j==3) reg[rt] = dDisk[i]&0xFF;
                        else if(j==2) reg[rt] = dDisk[i]>>8&0xFF;
                        else if(j==1) reg[rt] = dDisk[i]>>16&0xFF;
                        else reg[rt] = (unsigned int)dDisk[i]>>24;
                    }
                }
                else if(opcode==0x25) //lhu
                {
                    if(rt!=0)
                    {
                        if(j==2) reg[rt] = dDisk[i]&0xFFFF;
                        else reg[rt] = (unsigned int)dDisk[i]>>16;
                    }
                }
                else if(opcode==0x28)//sb
                {
                    if(j==3) dDisk[i] = (dDisk[i]&0xFFFFFF00) | (reg[rt]&0xFF);
                    else if(j==2) dDisk[i] = (dDisk[i]&0xFFFF00FF) | (reg[rt]<<8&0xFF00);
                    else if(j==1) dDisk[i] = (dDisk[i]&0xFF00FFFF) | (reg[rt]<<16&0xFF0000);
                    else dDisk[i] = (dDisk[i]&0xFFFFFF) | (reg[rt]<<24);
                }
                else if(opcode==0x29) //sh
                {
                    if(j==2) dDisk[i] = (dDisk[i]&0xFFFF0000) | (reg[rt]&0xFFFF);
                    else dDisk[i] = (dDisk[i]&0xFFFF) | (reg[rt]<<16);
                }
                else dDisk[i] = reg[rt]; //sw

                page = addr/DPoffest;
                for(i=0; i<DTsize; i++) DTLRU[i]++;
                for(i=0; i<DMsize; i++) DMLRU[i]++;

                for(i=0; i<DTsize; i++)
                {
                    if(DT[i]==page)
                    {
                        DThit++;
                        DTLRU[i]=0;
                        for(j=0; j<DMsize; j++)
                        {
                            if(DM[j]==page)
                            {
                                PPN = j;
                                break;
                            }
                        }
                        break;
                    }
                }
                if(i==DTsize)
                {
                    DTmiss++;
                    if(DP[page])
                    {
                        DPhit++;
                        for(i=0; i<DMsize; i++)
                        {
                            if(DM[i]==page)
                            {
                                DMLRU[i]=0;
                                PPN = i;
                                break;
                            }
                        }
                    }
                    else
                    {
                        DPmiss++;
                        DP[page]=1;
                        temp=0;
                        for(i=0; i<DMsize; i++)
                        {
                            if(DMLRU[i]>temp)
                            {
                                temp=DMLRU[i];
                                temp2=i;
                            }
                        }
                        temp = DM[temp2];
                        DM[temp2]=page;
                        DMLRU[temp2]=0;
                        PPN = temp2;

                        for(i=0; i<DCsize; i++)
                        {
                            if((DC[i]*DCoffest*DCmod/DPoffest)==PPN)
                            {
                                DC[i]=-1;
                                DMRU[i]=0;
                                DCvalid[i]=0;
                            }
                        }
                        for(i=0; i<DTsize; i++)
                        {
                            if(DT[i]==temp)
                            {
                                DT[i]=-1;
                                DTLRU[i]=1000000;
                            }
                        }
                        if(DP[temp]) DP[temp]=0;
                    }

                    temp=0;
                    for(i=0; i<DTsize; i++)
                    {
                        if(DTLRU[i]>temp)
                        {
                            temp=DTLRU[i];
                            temp2=i;
                        }
                    }
                    DT[temp2]=page;
                    DTLRU[temp2]=0;
                }

                PA = PPN*DPoffest + addr%DPoffest;
                set = PA/DCoffest%DCmod;
                tag = PA/DCoffest/DCmod;
                start = set*DCset;
                end = (set+1)*DCset;

                for(i=start; i<end; i++)
                {
                    if(DC[i]==tag)
                    {
                        DChit++;
                        for(j=start; j<end; j++) if(j!=i && DMRU[j]==0) break;
                        if(j==end) for(j=start; j<end; j++) DMRU[j]=0;
                        if(Dway!=1) DMRU[i]=1;
                        break;
                    }
                }

                if(i==end)
                {
                    DCmiss++;
                    for(i=start; i<end; i++)
                    {
                        if(DCvalid[i]==0)
                        {
                            DC[i]=tag;
                            for(j=start; j<end; j++) if(j!=i && DMRU[j]==0) break;
                            if(j==end) for(j=start; j<end; j++) DMRU[j]=0;
                            if(Dway!=1) DMRU[i]=1;
                            DCvalid[i]=1;
                            break;
                        }
                    }

                    if(i==end)
                    {
                        for(i=start; i<end; i++)
                        {
                            if(DMRU[i]==0)
                            {
                                DC[i]=tag;
                                for(j=start; j<end; j++) if(j!=i && DMRU[j]==0) break;
                                if(j==end) for(j=start; j<end; j++) DMRU[j]=0;
                                if(Dway!=1) DMRU[i]=1;
                                break;
                            }
                        }
                    }
                }
            }
            else break; //halt
        }
    }
    fprintf(report, "ICache :\n");
    fprintf(report, "# hits: %u\n", IChit);
    fprintf(report, "# misses: %u\n\n", ICmiss);
    fprintf(report, "DCache :\n");
    fprintf(report, "# hits: %u\n", DChit);
    fprintf(report, "# misses: %u\n\n", DCmiss);
    fprintf(report, "ITLB :\n");
    fprintf(report, "# hits: %u\n", IThit);
    fprintf(report, "# misses: %u\n\n", ITmiss);
    fprintf(report, "DTLB :\n");
    fprintf(report, "# hits: %u\n", DThit );
    fprintf(report, "# misses: %u\n\n", DTmiss);
    fprintf(report, "IPageTable :\n");
    fprintf(report, "# hits: %u\n", IPhit);
    fprintf(report, "# misses: %u\n\n", IPmiss);
    fprintf(report, "DPageTable :\n");
    fprintf(report, "# hits: %u\n", DPhit);
    fprintf(report, "# misses: %u\n\n", DPmiss);
    fclose(result);
    fclose(report);
    return 0;
}
int decode(unsigned int buff)
{
    int answer = 0;
    answer |= buff<<24;
    answer |= buff<<8&0xFF0000;
    answer |= buff>>8&0xFF00;
    answer |= buff>>24;
    return answer;
}
