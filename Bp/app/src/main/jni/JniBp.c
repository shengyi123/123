//
// Created by Admin on 2018/11/7.
//

#include <jni.h>
#include "com_admin_bp_JniBp.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "prosody_gen.h"

#define MaxSylNum 411
char code411_to_initial_char[MaxSylNum+1][16] = {"", "zh", "ch", "sh", "r", "z", "c", "s", "INULL", "zh", "ch", "sh", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "f", "INULL", "l", "b", "p", "m", "f", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "INULL", "zh", "ch", "sh", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "INULL", "zh", "sh", "z", "s", "g", "h", "d", "n", "l", "b", "p", "m", "f", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "p", "m", "f", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "f", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "n", "b", "p", "m", "f", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "f", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "f", "INULL", "j", "q", "x", "d", "t", "n", "l", "b", "p", "m", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "b", "p", "m", "f", "INULL", "j", "q", "x", "n", "l", "INULL", "j", "q", "x", "l", "INULL", "j", "q", "x", "d", "t", "n", "l", "b", "p", "m", "INULL", "INULL", "j", "q", "x", "d", "t", "n", "l", "b", "p", "m", "INULL", "j", "q", "x", "d", "n", "l", "m", "INULL", "j", "q", "x", "d", "t", "n", "l", "b", "p", "m", "INULL", "j", "q", "x", "n", "l", "b", "p", "m", "INULL", "j", "q", "x", "n", "l", "INULL", "j", "q", "x", "d", "t", "n", "l", "b", "p", "m", "INULL", "zh", "ch", "sh", "g", "k", "h", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "INULL", "zh", "ch", "sh", "g", "k", "h", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "INULL", "zh", "ch", "sh", "r", "z", "c", "s", "g", "k", "h", "d", "t", "l", "INULL", "zh", "ch", "sh", "g", "k", "h", "INULL", "zh", "ch", "r", "z", "c", "s", "g", "k", "h", "d", "t", "n", "l", "INULL", "j", "q", "x", "n", "l", "INULL", "j", "q", "x", "l", "INULL", "j", "q", "x", "l", "INULL", "j", "q", "x", "INULL", "INULL", "INULL", "INULL", "m"};
char code411_to_final_char[MaxSylNum+1][16] = {"", "FNULL1", "FNULL1", "FNULL1", "FNULL1", "FNULL2", "FNULL2", "FNULL2", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "a", "o", "o", "o", "o", "o", "o", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "e", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "ai", "eh", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ei", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ao", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "ou", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "an", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "en", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "ang", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "eng", "yi", "yi", "yi", "yi", "yi", "yi", "yi", "yi", "yi", "yi", "yi", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "wu", "yu", "yu", "yu", "yu", "yu", "yu", "ya", "ya", "ya", "ya", "ya", "ye", "ye", "ye", "ye", "ye", "ye", "ye", "ye", "ye", "ye", "ye", "yai", "yao", "yao", "yao", "yao", "yao", "yao", "yao", "yao", "yao", "yao", "yao", "you", "you", "you", "you", "you", "you", "you", "you", "yan", "yan", "yan", "yan", "yan", "yan", "yan", "yan", "yan", "yan", "yan", "yin", "yin", "yin", "yin", "yin", "yin", "yin", "yin", "yin", "yang", "yang", "yang", "yang", "yang", "yang", "ying", "ying", "ying", "ying", "ying", "ying", "ying", "ying", "ying", "ying", "ying", "wa", "wa", "wa", "wa", "wa", "wa", "wa", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wo", "wai", "wai", "wai", "wai", "wai", "wai", "wai", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wei", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wan", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wen", "wang", "wang", "wang", "wang", "wang", "wang", "wang", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "weng", "yue", "yue", "yue", "yue", "yue", "yue", "yuan", "yuan", "yuan", "yuan", "yuan", "yun", "yun", "yun", "yun", "yun", "yung", "yung", "yung", "yung", "er", "yo", "eng", "ei", "e"};

char Bstr[8][16] = {"B0", "B1", "B21", "B3", "B4", "B22", "B23", "Be"};


int Bptest(int argc, char *argv[]){


    FILE *fp_trans;// file pointer of transx
    FILE *fp_b;// file pointer of b

    //command line input arguments
    char transName[256] = {""};
    char bName[256] = {""};
    char *Bchar[BTypeNum] = {"B0", "B1", "B21", "B3", "B4", "B22", "B23"};
    float SR = 0.1;
    float PauTh = 0.025;
    int i = 0;
    int j = 0;
    int HTK_unit = 10000000;
    int cur_t = 0;
    int t = 0;
    char pre_context[16] = {""};
    char fol_context[16]= {""};
    char pre_B[16] = {""};
    char fol_B[16] = {""};

    //Data
    ProsodyDataSet p_data;

    // check arguments
    if( argc != 4 ) {
        printf("Usage: bp.exe  SR   *.tranx   *.b\n");
        system("PAUSE");
        return 1;
    }

    // get arguments from command line
    SR = atof(argv[1]);
    strcpy(transName, argv[2]);
    strcpy(bName, argv[3]);

    // load transx
    if( Trans2LinData(&p_data, transName) ) {
        printf("Cannot open %s\n", transName);
        system("PAUSE");
        return 0;
    }

    // open file pointer for savig b
    if( (fp_b = fopen(bName, "w")) == NULL ) {
        printf("Cannot save %s\n", bName);
        system("PAUSE");
        return 0;
    }

    // find leaf node index for the break-syntax model
    GetLeafNodeTag_forB(&p_data);
    // get probability of the break type on the leaf node found
    BreakPrediction(&p_data ,SR);

    j = 0;// index for syllable
    for(i=0;i<p_data.transx->size;i++) {
        fprintf(fp_b, "%s\t%d\t%d\t%d\t%d\t%d", p_data.transx->items[i].syl,
                p_data.transx->items[i].code, p_data.transx->items[i].index, p_data.transx->items[i].pos,
                p_data.transx->items[i].subindex, p_data.transx->items[i].subpos);
        if( p_data.transx->items[i].is_PM ) {
            fprintf(fp_b, "\tx\n");
        }
        else {
            fprintf(fp_b, "\t%s\n", Bstr[p_data.prosodys[j].B-1]);
            j ++;
        }
    }

    fclose(fp_b);
    return 0;
}
JNIEXPORT jint JNICALL Java_com_admin_bp_JniBp_Bp
        (JNIEnv *env, jobject obj, jint argcount, jstring argvector){
    int i;
    char **tmp;
    char *p;
    tmp = (char**)malloc(4* sizeof(char*)+4*1024*sizeof(char));
    for(i=0, p=(char*)(tmp+4);i<4;i++,p+=1024){
        tmp[i]=p;
    }
    tmp[0] = "/storage/emulated/0/Debug/bp.exe";
    tmp[1] = "0.19";
    tmp[2] = "/storage/emulated/0/0001.transx";
    tmp[3] = "/storage/emulated/0/0001.b";

    //const char *bp = (*env)->GetStringUTFChars(env,tmp,0);
    Bptest(4,tmp);
   // (*env)->ReleaseStringUTFChars(env,tmp,bp);
    return 0;
};