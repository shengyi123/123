//
// Created by Admin on 2018/11/7.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include"data_format.h"
#include"code411_merge_tables.h"
#include"break_tag_prediction_model.h"
#include"prosodic_state_tag_prediction_model.h"
#include"break_acoustic_model.h"
#include"SR_pause_table.h"
#include"syllable_prosodic_model.h"
#include"SR_norm_parameter.h"


void TRANSisPM(TRANS *p)
{
    int i;
    for(i=0;i<PM_Big5_Num;i++)
    {
        if( strcmp(p->syl, PM_Big5[i]) == 0 )
        {
            p->PM_id = PM_ID[i];
            p->is_PM = 1;
            return;
        }
    }
    p->is_PM = 0;
    return;
}

int Trans2LinData(ProsodyDataSet *p_data, char *transName){

    FILE *fp_trans;
    TRANS temp_trans;
    TRANS *transx;
    int n_trans = 0;
    int n_char = 0;
    int i = 0, j = 0;


    if( (fp_trans = fopen(transName,"r")) == NULL )
    {
        printf("Cannot open %s\n", transName);
        return 1;
    }

    while( fscanf(fp_trans,"%s %d %d %d %d %d",temp_trans.syl, &(temp_trans.code), &(temp_trans.index), &(temp_trans.pos), &(temp_trans.subindex), &(temp_trans.subpos)) == 6 )
    {
        n_trans ++;
    }
    fseek(fp_trans,0,SEEK_SET);
    transx = (TRANS *) calloc(n_trans, sizeof(TRANS));
    p_data->transx = (TRANSStream *) calloc(1, sizeof(TRANSStream));
    p_data->transx->items = transx;
    strcpy(p_data->transx->filename, transName);
    p_data->transx->size = n_trans;
    for(i=0;i<n_trans;i++){
        fscanf(fp_trans,"%s %d %d %d %d %d",transx[i].syl, &(transx[i].code), &(transx[i].index), &(transx[i].pos), &(transx[i].subindex), &(transx[i].subpos));
        //printf("%s %d %d %d %d %d\n",transx[i].syl, transx[i].code, transx[i].index, transx[i].pos, transx[i].subindex, transx[i].subpos);
        if( ((transx[i].code/1000) < 6 && (transx[i].code/1000) > 0) || ((int)(transx[i].code/1000)) == 7 || ((int)(transx[i].code/1000)) == 8 )
        {
            transx[i].syl_idx = n_char;
            n_char ++;
        }
        else {
            transx[i].syl_idx = -1;
        }
        TRANSisPM(&(transx[i]));
    }

    //printf("n_trans:%d  ,  c_cl:%d  \n",n_trans, n_char);

    p_data->DataNum = n_char;
    p_data->lins = (LinDataStream *) calloc(n_char, sizeof(LinDataStream));
    for(i=0;i<n_trans;i++){
        if( (transx[i].code%1000) < 1 || (transx[i].code%1000) > 877 )
        {
            printf("error 877 code in %s line:%d\n", transName, i+1);
            return 1;
        }
        if( (transx[i].code/1000)<6 || ((int)(transx[i].code/1000))==7 || ((int)(transx[i].code/1000))==8 )
        {
            p_data->lins[j].code411 = transx[i].code%1000;
            p_data->lins[j].tone = transx[i].code/1000;
            p_data->lins[j].index = transx[i].index;
            p_data->lins[j].subindex = transx[i].subindex;
            p_data->lins[j].pos = transx[i].pos;
            p_data->lins[j].subpos = transx[i].subpos;
            strcpy(p_data->lins[j].syl,transx[i].syl);

            if( (i+1)<n_trans )
            {
                if( transx[i+1].code >= 6000 && ((int)(transx[i+1].code/1000)) != 7  && ((int)(transx[i+1].code/1000)) != 8 )// there is a PM
                {
                    if( strcmp(transx[i+1].syl, "。") == 0 )
                    {
                        p_data->lins[j].PM = 1;
                    }
                    else
                    {
                        p_data->lins[j].PM = 2;
                    }
                    strcpy(p_data->lins[j].PMchar,transx[i+1].syl);
                }// No PM
                else
                {
                    p_data->lins[j].PM = 0;
                    strcpy(p_data->lins[j].PMchar,"None");

                }
            }
            j++;
        }

    }

    //printf("real syl:%d \n",j);
    //printf("lindata# :%s \n", p_data->lins[2].PMchar);


    fclose(fp_trans);
    //free(transx);
    return 0;
}



void GetLeafNodeTag_forB(ProsodyDataSet *p_data){

    int i = 0,j = 0, k = 0, l = 0, d = 0, m = 0, n = 0, M = 0, N = 0, len = 0, pre_len = 0, fol_len = 0, word_count = 0, tag = 0;
    int in_t, initial_type, initial_type_flag[8] = {0}, max_depth = 0, depth = 0;
    int juncture_flag = 0, type2_intraword_flag = 0, PM_flag[6] = {0};
    int L_S[30] = {0}, L_P_S[30] = {0}, L_F_S[30] = {0}, D_P_PM[15] = {0}, D_F_PM[15] = {0};
    int Right_POS_Window_Size = 1;
    int Left_POS_Window_Size = 1;
    int *code411_p_1, **LinFea;
    int **L_POS, **L_WL, **R_POS, **R_WL;



    d = 8+2+6+60+60+(96+4)*(Right_POS_Window_Size+Left_POS_Window_Size);

    // allocate memory
    code411_p_1 = (int *) calloc(p_data->DataNum, sizeof(int));
    code411_p_1[(p_data->DataNum)-1] = p_data->lins[0].code411;
    for(i=1;i<(p_data->DataNum);i++) code411_p_1[i-1] = p_data->lins[i].code411;

    LinFea= (int **)calloc((p_data->DataNum), sizeof(int*)) ;
    for(i=0;i<(p_data->DataNum);i++) LinFea[i] = (int*)calloc(d, sizeof(int))  ;

    //printf("test LinFea[5][6]: %d  , initial_type_flag[6]=%d \n",LinFea[5][6], initial_type_flag[6]);

    L_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) L_POS[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) R_POS[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;

    L_WL= (int **)calloc(4, sizeof(int*)) ;
    for(i=0;i<4;i++) L_WL[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_WL= (int **)calloc(4, sizeof(int*)) ;
    for(i=0;i<4;i++) R_WL[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;


    //////////////// linguistic features extraction //////////////////////////////////////////
    for(i=0 ; i<(p_data->DataNum) ; i++){

        // phonetic structure (8 flags)
        // following initial type
        for(j=0 ; j<8 ;j++)
            initial_type_flag[j] = 0;
        in_t = code411_to_initial[code411_p_1[i]-1];
        initial_type = code22_initial_to_7_classes[in_t-1];
        if( (initial_type==1) || (initial_type==4) )
            initial_type_flag[0]=1;
        initial_type_flag[initial_type] = 1;

        // interword, type-1 intraword or type-2 intraword (2 flags)
        if( ((p_data->lins[i].index)/100) == ((p_data->lins[i].index)%100) )
        {
            juncture_flag = 1;
            type2_intraword_flag = 0;
        }
        else
        {
            juncture_flag = 0;
            if( ((p_data->lins[i].subindex)/100) == ((p_data->lins[i].subindex)%100) )
                type2_intraword_flag = 1;
            else
                type2_intraword_flag = 0;
        }

        // PM: 6 flags
        for(j=0 ; j<6 ;j++)
            PM_flag[j] = 0;

        if(strcmp(p_data->lins[i].PMchar,"None")!=0)
        {
            PM_flag[0] = 1;
            if(strcmp(p_data->lins[i].PMchar,"、")==0)
                PM_flag[1] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"，")==0)
                PM_flag[2] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"？")==0)
                PM_flag[3] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"。")==0)
                PM_flag[4] = 1;
            else
                PM_flag[5] = 1;
        }
        //printf("strcmp PM:%d, %s, %s\n",strcmp(p_data->lins[i].PMchar,"None"), p_data->lins[i].PMchar, p_data->lins[i].syl );

        // Length of sentence (30 flags)
        // find Length of sentence /////////
        len = 0;
        for(j=i ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        for(j=(i-1) ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        if(len>=30)
            len = 30;
        //////////////////////////////////
        for(j=0 ; j<30 ;j++)
            L_S[j] = 0;
        for(j=0 ; j<len ;j++)
            L_S[j] = 1;





        // Length of previous sentence (30 flags)
        // find Length of previous sentence /////////
        pre_len = 0;
        len = 0;
        for(j=i ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        k = i - len;
        if(k<1)
            pre_len = 0;
        else
        {
            // find Length of sentence /////////
            pre_len = 0;
            for(j=k ; j<(p_data->DataNum) ; j++){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    pre_len++;
                else
                    break;
            }
            for(j=(k-1) ; j>=0 ; j--){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    pre_len++;
                else
                    break;
            }
            pre_len++;

            if(pre_len>=30)
                pre_len = 30;
            /////find Length of sentence end ///////////////////
        }


        for(j=0 ; j<30 ;j++)
            L_P_S[j] = 0;
        for(j=0 ; j<pre_len ;j++)
            L_P_S[j] = 1;
        //////////////find Length of previous sentence end ////////////////////


        // Length of following sentence (30 flags)
        // find Length of following sentence /////////
        fol_len = 0;
        len = 0;
        for(j=(i+1) ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        k = i + len;
        if(k>(p_data->DataNum))
            fol_len = 0;
        else
        {
            // find Length of sentence /////////
            fol_len = 0;
            for(j=k ; j<(p_data->DataNum) ; j++){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    fol_len++;
                else
                    break;
            }
            for(j=(k-1) ; j>=0 ; j--){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    fol_len++;
                else
                    break;
            }
            fol_len++;

            if(fol_len>=30)
                fol_len = 30;
            /////find Length of sentence end ///////////////////
        }


        for(j=0 ; j<30 ;j++)
            L_F_S[j] = 0;
        for(j=0 ; j<fol_len ;j++)
            L_F_S[j] = 1;
        //////////////find Length of following sentence end ////////////////////

        ////// Distance to previous PM (15 flags) //////////////////////
        // find length of previous///////
        len = 0;
        for(j=i ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        if(len>=15)
            len = 15;
        /////////find length of previous end//////
        for(j=0 ; j<15 ;j++)
            D_P_PM[j] = 0;
        for(j=0 ; j<len ;j++)
            D_P_PM[j] = 1;
        ////// Distance to previous PM end /// //////////////////////


        ////// Distance to following PM (15 flags) //////////////////////
        // find length of following///////
        len = 0;
        for(j=(i+1) ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        if(len>=15)
            len = 15;
        /////////find length of previous end//////
        for(j=0 ; j<15 ;j++)
            D_F_PM[j] = 0;
        for(j=0 ; j<len ;j++)
            D_F_PM[j] = 1;
        ////// Distance to previous PM end /// //////////////////////


        /////////// Left POS (96*Left_POS_Window_Size flags) //////////
        for(j=0;j<96;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_POS[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            L_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            L_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            L_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            L_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left POS (96*Left_POS_Window_Size flags) end /////

        /////////// Right POS (96*Left_POS_Window_Size flags) //////////
        for(j=0;j<96;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_POS[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            R_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            R_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            R_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            R_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Left POS (96*Left_POS_Window_Size flags) end /////

        /////////// Left Word Length (4*Left_POS_Window_Size flags) //////////
        for(j=0;j<4;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_WL[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<4)
                L_WL[len-1][word_count-1] = 1;
            else
                L_WL[4-1][word_count-1] = 1;
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left Word Length (4*Left_POS_Window_Size flags) end /////

        /////////// Right Word Length (4*Left_POS_Window_Size flags) //////////
        for(j=0;j<4;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_WL[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<4)
                R_WL[len-1][word_count-1] = 1;
            else
                R_WL[4-1][word_count-1] = 1;
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Right Word Length (4*Left_POS_Window_Size flags) end /////

        // merge flags
        j = 0;
        for(k=0;k<8;k++)
        {
            LinFea[i][j] = initial_type_flag[k]; //1~8 initial type
            j++;
        }
        LinFea[i][j] = juncture_flag; //interword /type-1 intraword
        j++;
        LinFea[i][j] = type2_intraword_flag; //type-2 intraword
        j++;
        for(k=0;k<6;k++)
        {
            LinFea[i][j] = PM_flag[k]; //11:is there a PM  12:頓號 13:逗號 14:問號 15:句號 16:其他
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_S[k]; //17:length of sentence is greater or equal to 1     46:length of sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_P_S[k]; //47:length of previous sentence is greater or equal to 1     76:length of previous sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_F_S[k]; //77:length of following sentence is greater or equal to 1     106:length of following sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<15;k++)
        {
            LinFea[i][j] = D_P_PM[k]; //107:distance to previous PM is greater or equal to 1     121:distance to previous PM is greater or equal to 15
            j++;
        }
        for(k=0;k<15;k++)
        {
            LinFea[i][j] = D_F_PM[k]; //122:distance to following PM is greater or equal to 1     136:distance to following PM is greater or equal to 15
            j++;
        }
        for(k=0;k<Left_POS_Window_Size;k++)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = L_POS[l][k];
                j++;
            }
        }
        for(k=0;k<Left_POS_Window_Size;k++)
        {
            for(l=0;l<4;l++)
            {
                LinFea[i][j] = L_WL[l][k];
                j++;
            }
        }
        for(k=0;k<Right_POS_Window_Size;k++)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = R_POS[l][k];
                j++;
            }
        }
        for(k=0;k<Right_POS_Window_Size;k++)
        {
            for(l=0;l<4;l++)
            {
                LinFea[i][j] = R_WL[l][k];
                j++;
            }
        }

        //for(k=0;k<d;k++)
        //	printf("%d ",LinFea[i][k]);
        //printf("\n");



    }// end of for(i=0 ; i<(p_data->DataNum) ; i++){
    //////////////////////////////////////////////////////////////////////////////

    /////  find leaf node tag from break-syntax model ///////////////////////////
    p_data->LeafNodeTag_forB = (int *) calloc(p_data->DataNum, sizeof(int));
    M = sizeof(SR_L4_Q)/sizeof(SR_L4_Q[0][0]);
    N = sizeof(SR_L4_Q[0])/sizeof(SR_L4_Q[0][0]); // node #
    M = M/N;                                      // Question #
    //printf("M:%d, N:%d\n",M,N);
    for(i=0 ; i<(p_data->DataNum) ; i++){
        //for(i=0 ; i<1 ; i++){
        tag = 0;
        max_depth = 0;
        for(n=0;n<N;n++)
        {
            depth = 0;
            for(m=0;m<M;m++)
            {
                if(SR_L4_Q[m][n]==0)
                    break;
                if( ((SR_L4_Q[m][n]>0) && (LinFea[i][abs(SR_L4_Q[m][n])-1]==1)) || ((SR_L4_Q[m][n]<0) && (LinFea[i][abs(SR_L4_Q[m][n])-1]==0)) )
                    depth++;
                else
                {
                    depth = 0;
                    break;
                }

            }
            if(depth > max_depth)
            {
                max_depth = depth;
                tag = n;
            }
            //printf("n=%d: tag=%d  depth=%d \n",(n+1), (tag+1), depth);
        }
        p_data->LeafNodeTag_forB[i] = tag + 1;
        //printf("tag %d: %d \n",(i+1), (tag+1));
    }



    free(code411_p_1);
    for(i=0;i<(p_data->DataNum);i++) free(LinFea[i]);
    free(LinFea);
    free(L_POS);
    free(R_POS);
    free(L_WL);
    free(R_WL);

}



void BreakPrediction(ProsodyDataSet *p_data, float SR){

    int i = 0, b = 0, k = 0, max_B = 0, p_tag = 0, leaf_node_num = 0;
    float score[BTypeNum] = {0.0}, max_score = 0.0;

    leaf_node_num = sizeof(SR_B_L4_leaf_node_index) / sizeof(SR_B_L4_leaf_node_index[0]);

    p_data->prosodys = (ProsodyStream *) calloc(p_data->DataNum, sizeof(ProsodyStream));

    ///////////// Viterbi ///////////
    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        // find leaf node location
        for(k=0;k<leaf_node_num;k++)
        {
            if(SR_B_L4_leaf_node_index[k] == p_data->LeafNodeTag_forB[i] )
            {
                p_tag = k;
                break;
            }
        }

        // calculate score for each break type: a[0]*SR + a[1]
        for(b=0;b<BTypeNum;b++)
        {
            score[b] = SR_B_L4_DeNd_leaf_node_P[b][0][p_tag]*SR + SR_B_L4_DeNd_leaf_node_P[b][1][p_tag];
            if(score[b] > 1.0)
                score[b] = 1.0;
            if(score[b] < 0.0)
                score[b] = 0.0;
        }

        // find maximum score
        max_score = score[0];
        max_B = 0;
        for(b=1;b<BTypeNum;b++)
        {
            if(score[b] > max_score)
            {
                max_score = score[b];
                max_B = b;
            }
        }

        p_data->prosodys[i].B = max_B + 1;
        //printf("%d: B=%d\n",(i+1),(max_B+1));

    }

    p_data->prosodys[(p_data->DataNum)-1].B = BTypeNum + 1;
    //p_data->prosodys[(p_data->DataNum)-1].B = 5; //B4





}



void GetLeafNodeTag_forPS(ProsodyDataSet *p_data){

    int i = 0,j = 0, k = 0, l = 0, d = 0, c_w_l = 0, s_p_w = 0, word_count = 0;
    int PM_flag[6] = {0}, len = 0, b_len = 0, e_len = 0, uttb = 0, utte = 0;
    int S_P_W[4] = {0}, L_S[6] = {0}, S_P_S[13] = {0};
    int Right_POS_Window_Size = 2;
    int Left_POS_Window_Size = 3;
    int leaf_node_index = 0, new_leaf_node_index = 0, QA = 0;
    int **LinFea;
    int **L_POS, **L_WL, **R_POS, **R_WL;



    d = 31+101*(Right_POS_Window_Size+Left_POS_Window_Size);

    // allocate memory

    LinFea= (int **)calloc((p_data->DataNum), sizeof(int*)) ;
    for(i=0;i<(p_data->DataNum);i++) LinFea[i] = (int*)calloc(d, sizeof(int))  ;


    L_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) L_POS[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) R_POS[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;

    L_WL= (int **)calloc(5, sizeof(int*)) ;
    for(i=0;i<5;i++) L_WL[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_WL= (int **)calloc(5, sizeof(int*)) ;
    for(i=0;i<5;i++) R_WL[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;


    //////////////// linguistic features extraction //////////////////////////////////////////
    for(i=0 ; i<(p_data->DataNum) ; i++){

        ////////  syllable position in a word ///////////////////////////////
        c_w_l = ((p_data->lins[i].index) / 100); // current word length

        for(j=0;j<4;j++)
            S_P_W[j] = 0; // 1st, intermediate, last, mono

        if(c_w_l==1)
            S_P_W[4-1] = 1;
        else
        {
            s_p_w = ((p_data->lins[i].index) % 100);
            if(s_p_w==1)
                S_P_W[1-1] = 1;
            else
            {
                if(s_p_w==c_w_l)
                    S_P_W[3-1] = 1;
                else
                    S_P_W[2-1] = 1;
            }
        }

        ////////  syllable position in a word end ///////////////////////////////

        ///////  sentence length //////////////////////////////////////////////
        // find Length of sentence /////////
        len = 0;
        for(j=i ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        for(j=(i-1) ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        //////////////////////////////////
        for(j=0;j<6;j++)
            L_S[j] = 0;

        if(len==1)
            L_S[1-1] = 1;
        else
        if( (len>=1) && (len<=5) )
            L_S[2-1] = 1;
        else
        if( (len>=6) && (len<=10) )
            L_S[3-1] = 1;
        else
        if( (len>=11) && (len<=15) )
            L_S[4-1] = 1;
        else
        if( (len>=16) && (len<=20) )
            L_S[5-1] = 1;
        else
            L_S[6-1] = 1;

        ///////  sentence length end ///////////////////////////////////////////

        ///////  syllable position in a sentence //////////////////////////////////////////////
        // find distance_to_previous_PM /////////
        b_len = 0;
        for(j=i ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                b_len++;
            else
                break;
        }
        // find distance_to_following_PM /////////
        e_len = 0;
        for(j=(i+1) ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                e_len++;
            else
                break;
        }
        e_len++;
        ////////////////////////////////////////
        e_len++;
        for(j=0;j<13;j++)
            S_P_S[j] = 0;

        if(b_len > e_len)
        {
            if(e_len==1) // last syllable
                S_P_S[7-1] = 1;
            else
            if(e_len==2)
                S_P_S[8-1] = 1;
            else
            if(e_len==3)
                S_P_S[9-1] = 1;
            else
            if( (e_len>=4) && (e_len<=5) )
                S_P_S[10-1] = 1;
            else
            if( (e_len>=6) && (e_len<=7) )
                S_P_S[11-1] = 1;
            else
            if( (e_len>=8) && (e_len<=11) )
                S_P_S[12-1] = 1;
            else
                S_P_S[13-1] = 1;
        }
        else
        {
            if(b_len==1)
                S_P_S[1-1] = 1;
            else
            if(b_len==2)
                S_P_S[2-1] = 1;
            else
            if(b_len==3)
                S_P_S[3-1] = 1;
            else
            if( (b_len>=4) && (b_len<=5) )
                S_P_S[4-1] = 1;
            else
            if( (b_len>=6) && (b_len<=7) )
                S_P_S[5-1] = 1;
            else
            if( (b_len>=8) && (b_len<=11) )
                S_P_S[6-1] = 1;
            else
                S_P_S[13-1] = 1;
        }
        ///////  syllable position in a sentence end //////////////////////////////////////////

        /////////////// PM: 6 flags //////////////////////////////////////////////////////////
        for(j=0 ; j<6 ;j++)
            PM_flag[j] = 0;

        if(strcmp(p_data->lins[i].PMchar,"None")!=0)
        {
            PM_flag[0] = 1;
            if(strcmp(p_data->lins[i].PMchar,"、")==0)
                PM_flag[1] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"，")==0)
                PM_flag[2] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"？")==0)
                PM_flag[3] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"。")==0)
                PM_flag[4] = 1;
            else
                PM_flag[5] = 1;
        }
        /////////////// PM: 6 flags end ///////////////////////////////////////////////////////

        /////////// Left POS (96*Left_POS_Window_Size flags) /////////////////////////////////
        for(j=0;j<96;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_POS[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            L_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            L_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            L_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            L_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left POS (96*Left_POS_Window_Size flags) end /////

        /////////// Right POS (96*Left_POS_Window_Size flags) //////////
        for(j=0;j<96;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_POS[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            R_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            R_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            R_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            R_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Right POS (96*Left_POS_Window_Size flags) end /////

        /////////// Left Word Length (5*Left_POS_Window_Size flags) //////////
        for(j=0;j<5;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_WL[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<5)
                L_WL[len-1][word_count-1] = 1;
            else
                L_WL[5-1][word_count-1] = 1;
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left Word Length (5*Left_POS_Window_Size flags) end /////

        /////////// Right Word Length (5*Left_POS_Window_Size flags) //////////
        for(j=0;j<5;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_WL[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<5)
                R_WL[len-1][word_count-1] = 1;
            else
                R_WL[5-1][word_count-1] = 1;
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Right Word Length (5*Left_POS_Window_Size flags) end /////

        if(i==0)
            uttb = 1;
        else
            uttb = 0;

        if( i==((p_data->DataNum)-1) )
            utte = 1;
        else
            utte = 0;



        // merge flags
        j = 0;
        for(k=0;k<4;k++)
        {
            LinFea[i][j] = S_P_W[k]; // 1~4 syllable position in a word
            j++;
        }
        for(k=0;k<6;k++)
        {
            LinFea[i][j] = L_S[k]; // 5~10 sentence length
            j++;
        }
        for(k=0;k<13;k++)
        {
            LinFea[i][j] = S_P_S[k]; // 11:23 syllable position in a sentence
            j++;
        }
        for(k=0;k<6;k++)
        {
            LinFea[i][j] = PM_flag[k]; // 24:29 PM type
            j++;
        }
        LinFea[i][j] = uttb;
        j++;
        LinFea[i][j] = utte;
        j++;



        for(k=(Left_POS_Window_Size-1);k>=0;k--)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = L_POS[l][k];
                j++;
            }
            for(l=0;l<5;l++)
            {
                LinFea[i][j] = L_WL[l][k];
                j++;
            }
        }

        for(k=0;k<Right_POS_Window_Size;k++)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = R_POS[l][k];
                j++;
            }
            for(l=0;l<5;l++)
            {
                LinFea[i][j] = R_WL[l][k];
                j++;
            }
        }


        //for(k=0;k<d;k++)
        //	printf("%d ",LinFea[i][k]);
        //printf("\n");



    }// end of for(i=0 ; i<(p_data->DataNum) ; i++){
    //////////////////////////////////////////////////////////////////////////////


    /////  find leaf node tag from prosodic state-syntax model ///////////////////////////
    p_data->LeafNodeTag_for_p = (int *) calloc(p_data->DataNum, sizeof(int));
    p_data->LeafNodeTag_for_q = (int *) calloc(p_data->DataNum, sizeof(int));
    p_data->LeafNodeTag_for_r = (int *) calloc(p_data->DataNum, sizeof(int));


    //for(i=0 ; i<1 ; i++)
    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        ///////// p part //////////
        leaf_node_index = 1;
        while( p_L_Tree[leaf_node_index-1] != (-1) )
        {
            QA = LinFea[i][p_L_Tree[leaf_node_index-1]-1];
            if(QA==1)
                new_leaf_node_index = p_L_son[leaf_node_index-1];
            else
                new_leaf_node_index = p_L_son[leaf_node_index-1] + 1;

            if( p_L_Tree[new_leaf_node_index-1] == (-1) )
            {
                leaf_node_index = new_leaf_node_index;
                break;
            }
            else
                leaf_node_index = new_leaf_node_index;

        }
        p_data->LeafNodeTag_for_p[i] = leaf_node_index;

        ///////// q part //////////
        leaf_node_index = 1;
        while( q_L_Tree[leaf_node_index-1] != (-1))
        {
            QA = LinFea[i][q_L_Tree[leaf_node_index-1]-1];
            if(QA==1)
                new_leaf_node_index = q_L_son[leaf_node_index-1];
            else
                new_leaf_node_index = q_L_son[leaf_node_index-1] + 1;

            if( q_L_Tree[new_leaf_node_index-1] == (-1) )
            {
                leaf_node_index = new_leaf_node_index;
                break;
            }
            else
                leaf_node_index = new_leaf_node_index;

        }
        p_data->LeafNodeTag_for_q[i] = leaf_node_index;

        ///////// r part //////////
        leaf_node_index = 1;
        while( r_L_Tree[leaf_node_index-1] != (-1))
        {
            QA = LinFea[i][r_L_Tree[leaf_node_index-1]-1];
            if(QA==1)
                new_leaf_node_index = r_L_son[leaf_node_index-1];
            else
                new_leaf_node_index = r_L_son[leaf_node_index-1] + 1;

            if( r_L_Tree[new_leaf_node_index-1] == (-1) )
            {
                leaf_node_index = new_leaf_node_index;
                break;
            }
            else
                leaf_node_index = new_leaf_node_index;

        }
        p_data->LeafNodeTag_for_r[i] = leaf_node_index;




        //printf("%d: p=%d q=%d r=%d\n",(i+1), p_data->LeafNodeTag_for_p[i],p_data->LeafNodeTag_for_q[i],p_data->LeafNodeTag_for_r[i]);
    }




    free(LinFea);
    free(L_POS);
    free(R_POS);
    free(L_WL);
    free(R_WL);

}


void ProsodicStatePrediction_p(ProsodyDataSet *p_data, float SR){

    int i = 0, j = 0, k = 0, Bin_index = 0, Bin_num = 0, max_state = 0;
    float dist = 0.0, min_dist = 0.0;
    double VERY_SMALL_VAL = 1.0e-100;
    double **score;
    int **from;
    double PS_score = 0.0, L_score = 0.0, max_score = 0.0;

    // allocate memory
    score = (double **)calloc(PSNum, sizeof(double*)) ;
    for(i=0;i<PSNum;i++) score[i] = (double*)calloc((p_data->DataNum), sizeof(double))  ;

    from = (int **)calloc(PSNum, sizeof(int*)) ;
    for(i=0;i<PSNum;i++) from[i] = (int*)calloc((p_data->DataNum), sizeof(int))  ;




    // find SR bin index
    Bin_num = sizeof(SR_Bin) / sizeof(SR_Bin[0]);
    min_dist = fabs( (SR - SR_Bin[0]) );
    Bin_index = 0 + 1;
    for(i=1;i<Bin_num;i++)
    {
        dist = fabs( (SR - SR_Bin[i]) );
        if(dist < min_dist)
        {
            min_dist = dist;
            Bin_index = i + 1;
        }
    }

    // forward
    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        if(i==0)
        {
            for(j=0;j<PSNum;j++)
            {
                PS_score = SR_P_PS_initial_Bin[Bin_index-1][j];
                L_score = p_L_P[j][p_data->LeafNodeTag_for_p[i]-1];
                if(PS_score<=VERY_SMALL_VAL)
                    PS_score = log(VERY_SMALL_VAL);
                else
                    PS_score = log(PS_score);

                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                score[j][i] = PS_score + L_score;
            }
        }
        else
        {
            for(j=0;j<PSNum;j++)
            {
                L_score = p_L_P[j][p_data->LeafNodeTag_for_p[i]-1];
                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                for(k=0;k<PSNum;k++)
                {
                    if(k==0)
                    {
                        PS_score = SR_P_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        max_state = k;
                        max_score = score[k][i-1] + PS_score;
                    }
                    else
                    {
                        PS_score = SR_P_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        if( (score[k][i-1] + PS_score) > max_score )
                        {
                            max_state = k;
                            max_score = score[k][i-1] + PS_score;
                        }
                    }
                } // end of for(k=0;k<PSNum;k++)
                score[j][i] = max_score + L_score;
                from[j][i] = max_state;
            } // end of for(j=0;j<PSNum;j++)

        } // end of else

    }// end



    ///////////////// back trace
    for(j=0;j<PSNum;j++)
    {
        //printf("%d score: %.10f \n", (j+1), score[j][(p_data->DataNum)-1]);

        if(j==0)
        {
            max_score = score[j][(p_data->DataNum)-1];
            max_state = j ;
        }
        else
        {
            if( score[j][(p_data->DataNum)-1] > max_score )
            {
                max_score = score[j][(p_data->DataNum)-1];
                max_state = j;
            }
        }
    }
    p_data->prosodys[(p_data->DataNum)-1].PS_p = max_state + 1;
    for(i=1 ; i<=((p_data->DataNum)-1) ; i++)
    {
        k = p_data->prosodys[(p_data->DataNum)-1-i+1].PS_p;
        p_data->prosodys[(p_data->DataNum)-1-i].PS_p = from[k-1][(p_data->DataNum)-1-i+1] + 1;
    }

    //for(i=0 ; i<(p_data->DataNum) ; i++)
    //printf("%d: p=%d\n",i+1, p_data->prosodys[i].PS_p);

    free(score);
    free(from);
}


void ProsodicStatePrediction_q(ProsodyDataSet *p_data, float SR){

    int i = 0, j = 0, k = 0, Bin_index = 0, Bin_num = 0, max_state = 0;
    float dist = 0.0, min_dist = 0.0;
    double VERY_SMALL_VAL = 1.0e-100;
    double **score;
    int **from;
    double PS_score = 0.0, L_score = 0.0, max_score = 0.0;

    // allocate memory
    score = (double **)calloc(PSNum, sizeof(double*)) ;
    for(i=0;i<PSNum;i++) score[i] = (double*)calloc((p_data->DataNum), sizeof(double))  ;

    from = (int **)calloc(PSNum, sizeof(int*)) ;
    for(i=0;i<PSNum;i++) from[i] = (int*)calloc((p_data->DataNum), sizeof(int))  ;




    // find SR bin index
    Bin_num = sizeof(SR_Bin) / sizeof(SR_Bin[0]);
    min_dist = fabs( (SR - SR_Bin[0]) );
    Bin_index = 0 + 1;
    for(i=1;i<Bin_num;i++)
    {
        dist = fabs( (SR - SR_Bin[i]) );
        if(dist < min_dist)
        {
            min_dist = dist;
            Bin_index = i + 1;
        }
    }

    //printf("bin_index = %d \n",Bin_index);

    // forward
    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        if(i==0)
        {
            for(j=0;j<PSNum;j++)
            {
                PS_score = SR_D_PS_initial_Bin[Bin_index-1][j];
                L_score = q_L_P[j][p_data->LeafNodeTag_for_q[i]-1];
                if(PS_score<=VERY_SMALL_VAL)
                    PS_score = log(VERY_SMALL_VAL);
                else
                    PS_score = log(PS_score);

                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                score[j][i] = PS_score + L_score;
            }
        }
        else
        {
            for(j=0;j<PSNum;j++)
            {
                L_score = q_L_P[j][p_data->LeafNodeTag_for_q[i]-1];
                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                for(k=0;k<PSNum;k++)
                {
                    if(k==0)
                    {
                        PS_score = SR_D_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        max_state = k;
                        max_score = score[k][i-1] + PS_score;
                    }
                    else
                    {
                        PS_score = SR_D_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        if( (score[k][i-1] + PS_score) > max_score )
                        {
                            max_state = k;
                            max_score = score[k][i-1] + PS_score;
                        }
                    }
                } // end of for(k=0;k<PSNum;k++)
                score[j][i] = max_score + L_score;
                from[j][i] = max_state;
            } // end of for(j=0;j<PSNum;j++)

        } // end of else

    }// end



    ///////////////// back trace
    for(j=0;j<PSNum;j++)
    {
        //printf("%d score: %.10f \n", (j+1), score[j][(p_data->DataNum)-1]);

        if(j==0)
        {
            max_score = score[j][(p_data->DataNum)-1];
            max_state = j ;
        }
        else
        {
            if( score[j][(p_data->DataNum)-1] > max_score )
            {
                max_score = score[j][(p_data->DataNum)-1];
                max_state = j;
            }
        }
    }
    p_data->prosodys[(p_data->DataNum)-1].PS_q = max_state + 1;
    for(i=1 ; i<=((p_data->DataNum)-1) ; i++)
    {
        k = p_data->prosodys[(p_data->DataNum)-1-i+1].PS_q;
        p_data->prosodys[(p_data->DataNum)-1-i].PS_q = from[k-1][(p_data->DataNum)-1-i+1] + 1;
    }

    //for(i=0 ; i<(p_data->DataNum) ; i++)
    //printf("%d: q=%d\n",i+1, p_data->prosodys[i].PS_q);

    free(score);
    free(from);
}


void ProsodicStatePrediction_r(ProsodyDataSet *p_data, float SR){

    int i = 0, j = 0, k = 0, Bin_index = 0, Bin_num = 0, max_state = 0;
    float dist = 0.0, min_dist = 0.0;
    double VERY_SMALL_VAL = 1.0e-100;
    double **score;
    int **from;
    double PS_score = 0.0, L_score = 0.0, max_score = 0.0;

    // allocate memory
    score = (double **)calloc(PSNum, sizeof(double*)) ;
    for(i=0;i<PSNum;i++) score[i] = (double*)calloc((p_data->DataNum), sizeof(double))  ;

    from = (int **)calloc(PSNum, sizeof(int*)) ;
    for(i=0;i<PSNum;i++) from[i] = (int*)calloc((p_data->DataNum), sizeof(int))  ;




    // find SR bin index
    Bin_num = sizeof(SR_Bin) / sizeof(SR_Bin[0]);
    min_dist = fabs( (SR - SR_Bin[0]) );
    Bin_index = 0 + 1;
    for(i=1;i<Bin_num;i++)
    {
        dist = fabs( (SR - SR_Bin[i]) );
        if(dist < min_dist)
        {
            min_dist = dist;
            Bin_index = i + 1;
        }
    }

    // forward
    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        if(i==0)
        {
            for(j=0;j<PSNum;j++)
            {
                PS_score = SR_E_PS_initial_Bin[Bin_index-1][j];
                L_score = r_L_P[j][p_data->LeafNodeTag_for_r[i]-1];
                if(PS_score<=VERY_SMALL_VAL)
                    PS_score = log(VERY_SMALL_VAL);
                else
                    PS_score = log(PS_score);

                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                score[j][i] = PS_score + L_score;
            }
        }
        else
        {
            for(j=0;j<PSNum;j++)
            {
                L_score = r_L_P[j][p_data->LeafNodeTag_for_r[i]-1];
                if(L_score<=VERY_SMALL_VAL)
                    L_score = log(VERY_SMALL_VAL);
                else
                    L_score = log(L_score);

                for(k=0;k<PSNum;k++)
                {
                    if(k==0)
                    {
                        PS_score = SR_E_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        max_state = k;
                        max_score = score[k][i-1] + PS_score;
                    }
                    else
                    {
                        PS_score = SR_E_PS_Bx_Bin[Bin_index-1][k][j][(p_data->prosodys[i-1].B)-1];
                        if(PS_score<=VERY_SMALL_VAL)
                            PS_score = log(VERY_SMALL_VAL);
                        else
                            PS_score = log(PS_score);
                        if( (score[k][i-1] + PS_score) > max_score )
                        {
                            max_state = k;
                            max_score = score[k][i-1] + PS_score;
                        }
                    }
                } // end of for(k=0;k<PSNum;k++)
                score[j][i] = max_score + L_score;
                from[j][i] = max_state;
            } // end of for(j=0;j<PSNum;j++)

        } // end of else

    }// end



    ///////////////// back trace
    for(j=0;j<PSNum;j++)
    {
        //printf("%d score: %.10f \n", (j+1), score[j][(p_data->DataNum)-1]);

        if(j==0)
        {
            max_score = score[j][(p_data->DataNum)-1];
            max_state = j ;
        }
        else
        {
            if( score[j][(p_data->DataNum)-1] > max_score )
            {
                max_score = score[j][(p_data->DataNum)-1];
                max_state = j;
            }
        }
    }
    p_data->prosodys[(p_data->DataNum)-1].PS_r = max_state + 1;
    for(i=1 ; i<=((p_data->DataNum)-1) ; i++)
    {
        k = p_data->prosodys[(p_data->DataNum)-1-i+1].PS_r;
        p_data->prosodys[(p_data->DataNum)-1-i].PS_r = from[k-1][(p_data->DataNum)-1-i+1] + 1;
    }

    //for(i=0 ; i<(p_data->DataNum) ; i++)
    //printf("%d: r=%d\n",i+1, p_data->prosodys[i].PS_r);

    free(score);
    free(from);
}


void PausePrediction(ProsodyDataSet *p_data, float SR)
{
    int i = 0,j = 0, k = 0, l = 0, d = 0, m = 0, n = 0, M = 0, N = 0, len = 0, pre_len = 0, fol_len = 0, word_count = 0, tag = 0;
    int in_t, initial_type, initial_type_flag[8] = {0}, max_depth = 0, depth = 0, max_bin_index = 0;
    int juncture_flag = 0, type2_intraword_flag = 0, PM_flag[6] = {0};
    int L_S[30] = {0}, L_P_S[30] = {0}, L_F_S[30] = {0}, D_P_PM[15] = {0}, D_F_PM[15] = {0};
    int Right_POS_Window_Size = 1;
    int Left_POS_Window_Size = 1;
    int *code411_p_1, **LinFea;
    int **L_POS, **L_WL, **R_POS, **R_WL;
    int *Tags;
    float dist = 0.0, max_dist = 0.0;



    d = 8+2+6+60+60+(96+4)*(Right_POS_Window_Size+Left_POS_Window_Size);

    // allocate memory
    code411_p_1 = (int *) calloc(p_data->DataNum, sizeof(int));
    code411_p_1[(p_data->DataNum)-1] = p_data->lins[0].code411;
    for(i=1;i<(p_data->DataNum);i++) code411_p_1[i-1] = p_data->lins[i].code411;

    LinFea= (int **)calloc((p_data->DataNum), sizeof(int*)) ;
    for(i=0;i<(p_data->DataNum);i++) LinFea[i] = (int*)calloc(d, sizeof(int))  ;


    L_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) L_POS[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_POS= (int **)calloc(96, sizeof(int*)) ;
    for(i=0;i<96;i++) R_POS[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;

    L_WL= (int **)calloc(4, sizeof(int*)) ;
    for(i=0;i<4;i++) L_WL[i] = (int*)calloc(Left_POS_Window_Size, sizeof(int))  ;

    R_WL= (int **)calloc(4, sizeof(int*)) ;
    for(i=0;i<4;i++) R_WL[i] = (int*)calloc(Right_POS_Window_Size, sizeof(int))  ;


    //////////////// linguistic features extraction //////////////////////////////////////////
    for(i=0 ; i<(p_data->DataNum) ; i++){

        // phonetic structure (8 flags)
        // following initial type
        for(j=0 ; j<8 ;j++)
            initial_type_flag[j] = 0;
        in_t = code411_to_initial[code411_p_1[i]-1];
        initial_type = code22_initial_to_7_classes[in_t-1];
        if( (initial_type==1) || (initial_type==4) )
            initial_type_flag[0]=1;
        initial_type_flag[initial_type] = 1;

        // interword, type-1 intraword or type-2 intraword (2 flags)
        if( ((p_data->lins[i].index)/100) == ((p_data->lins[i].index)%100) )
        {
            juncture_flag = 1;
            type2_intraword_flag = 0;
        }
        else
        {
            juncture_flag = 0;
            if( ((p_data->lins[i].subindex)/100) == ((p_data->lins[i].subindex)%100) )
                type2_intraword_flag = 1;
            else
                type2_intraword_flag = 0;
        }

        // PM: 6 flags
        for(j=0 ; j<6 ;j++)
            PM_flag[j] = 0;

        if(strcmp(p_data->lins[i].PMchar,"None")!=0)
        {
            PM_flag[0] = 1;
            if(strcmp(p_data->lins[i].PMchar,"、")==0)
                PM_flag[1] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"，")==0)
                PM_flag[2] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"？")==0)
                PM_flag[3] = 1;
            else if(strcmp(p_data->lins[i].PMchar,"。")==0)
                PM_flag[4] = 1;
            else
                PM_flag[5] = 1;
        }
        //printf("strcmp PM:%d, %s, %s\n",strcmp(p_data->lins[i].PMchar,"None"), p_data->lins[i].PMchar, p_data->lins[i].syl );

        // Length of sentence (30 flags)
        // find Length of sentence /////////
        len = 0;
        for(j=i ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        for(j=(i-1) ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        if(len>=30)
            len = 30;
        //////////////////////////////////
        for(j=0 ; j<30 ;j++)
            L_S[j] = 0;
        for(j=0 ; j<len ;j++)
            L_S[j] = 1;





        // Length of previous sentence (30 flags)
        // find Length of previous sentence /////////
        pre_len = 0;
        len = 0;
        for(j=i ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        k = i - len;
        if(k<1)
            pre_len = 0;
        else
        {
            // find Length of sentence /////////
            pre_len = 0;
            for(j=k ; j<(p_data->DataNum) ; j++){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    pre_len++;
                else
                    break;
            }
            for(j=(k-1) ; j>=0 ; j--){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    pre_len++;
                else
                    break;
            }
            pre_len++;

            if(pre_len>=30)
                pre_len = 30;
            /////find Length of sentence end ///////////////////
        }


        for(j=0 ; j<30 ;j++)
            L_P_S[j] = 0;
        for(j=0 ; j<pre_len ;j++)
            L_P_S[j] = 1;
        //////////////find Length of previous sentence end ////////////////////


        // Length of following sentence (30 flags)
        // find Length of following sentence /////////
        fol_len = 0;
        len = 0;
        for(j=(i+1) ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        k = i + len;
        if(k>(p_data->DataNum))
            fol_len = 0;
        else
        {
            // find Length of sentence /////////
            fol_len = 0;
            for(j=k ; j<(p_data->DataNum) ; j++){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    fol_len++;
                else
                    break;
            }
            for(j=(k-1) ; j>=0 ; j--){
                if(strcmp(p_data->lins[j].PMchar,"None")==0)
                    fol_len++;
                else
                    break;
            }
            fol_len++;

            if(fol_len>=30)
                fol_len = 30;
            /////find Length of sentence end ///////////////////
        }


        for(j=0 ; j<30 ;j++)
            L_F_S[j] = 0;
        for(j=0 ; j<fol_len ;j++)
            L_F_S[j] = 1;
        //////////////find Length of following sentence end ////////////////////

        ////// Distance to previous PM (15 flags) //////////////////////
        // find length of previous///////
        len = 0;
        for(j=i ; j>=0 ; j--){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        if(len>=15)
            len = 15;
        /////////find length of previous end//////
        for(j=0 ; j<15 ;j++)
            D_P_PM[j] = 0;
        for(j=0 ; j<len ;j++)
            D_P_PM[j] = 1;
        ////// Distance to previous PM end /// //////////////////////


        ////// Distance to following PM (15 flags) //////////////////////
        // find length of following///////
        len = 0;
        for(j=(i+1) ; j<(p_data->DataNum) ; j++){
            if(strcmp(p_data->lins[j].PMchar,"None")==0)
                len++;
            else
                break;
        }
        len++;
        if(len>=15)
            len = 15;
        /////////find length of previous end//////
        for(j=0 ; j<15 ;j++)
            D_F_PM[j] = 0;
        for(j=0 ; j<len ;j++)
            D_F_PM[j] = 1;
        ////// Distance to previous PM end /// //////////////////////


        /////////// Left POS (96*Left_POS_Window_Size flags) //////////
        for(j=0;j<96;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_POS[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            L_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            L_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            L_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            L_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left POS (96*Left_POS_Window_Size flags) end /////

        /////////// Right POS (96*Left_POS_Window_Size flags) //////////
        for(j=0;j<96;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_POS[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            R_POS[     POS_Level[p_data->lins[k].pos-1][1-1]-1][word_count-1] = 1; //level 0 POS
            R_POS[3+   POS_Level[p_data->lins[k].pos-1][2-1]-1][word_count-1] = 1; //level 1 POS
            R_POS[3+12+POS_Level[p_data->lins[k].pos-1][3-1]-1][word_count-1] = 1; // level 2 POS
            R_POS[3+12+23+p_data->lins[k].pos-1][word_count-1] = 1; //level 3 POS
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Left POS (96*Left_POS_Window_Size flags) end /////

        /////////// Left Word Length (4*Left_POS_Window_Size flags) //////////
        for(j=0;j<4;j++)
            for(l=0;l<Left_POS_Window_Size;l++)
                L_WL[j][l] = 0;
        k = i;
        word_count = 1;
        while( (k>=0) && (word_count<=Left_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<4)
                L_WL[len-1][word_count-1] = 1;
            else
                L_WL[4-1][word_count-1] = 1;
            k = k - ((p_data->lins[k].index) % 100);
            word_count = word_count + 1;
        }
        /////////// Left Word Length (4*Left_POS_Window_Size flags) end /////

        /////////// Right Word Length (4*Left_POS_Window_Size flags) //////////
        for(j=0;j<4;j++)
            for(l=0;l<Right_POS_Window_Size;l++)
                R_WL[j][l] = 0;
        k = i + ( ((p_data->lins[i].index)/100) - ((p_data->lins[i].index)%100) ) + 1;
        word_count = 1;
        while( (k<(p_data->DataNum)) && (word_count<=Right_POS_Window_Size)   )
        {
            len = p_data->lins[k].index / 100;
            if(len<4)
                R_WL[len-1][word_count-1] = 1;
            else
                R_WL[4-1][word_count-1] = 1;
            k = k + ( ((p_data->lins[k].index)/100) - ((p_data->lins[k].index)%100) ) + 1;
            word_count = word_count + 1;
        }
        /////////// Right Word Length (4*Left_POS_Window_Size flags) end /////

        // merge flags
        j = 0;
        for(k=0;k<8;k++)
        {
            LinFea[i][j] = initial_type_flag[k]; //1~8 initial type
            j++;
        }
        LinFea[i][j] = juncture_flag; //interword /type-1 intraword
        j++;
        LinFea[i][j] = type2_intraword_flag; //type-2 intraword
        j++;
        for(k=0;k<6;k++)
        {
            LinFea[i][j] = PM_flag[k]; //11:is there a PM  12:頓號 13:逗號 14:問號 15:句號 16:其他
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_S[k]; //17:length of sentence is greater or equal to 1     46:length of sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_P_S[k]; //47:length of previous sentence is greater or equal to 1     76:length of previous sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<30;k++)
        {
            LinFea[i][j] = L_F_S[k]; //77:length of following sentence is greater or equal to 1     106:length of following sentence is greater or equal to 30
            j++;
        }
        for(k=0;k<15;k++)
        {
            LinFea[i][j] = D_P_PM[k]; //107:distance to previous PM is greater or equal to 1     121:distance to previous PM is greater or equal to 15
            j++;
        }
        for(k=0;k<15;k++)
        {
            LinFea[i][j] = D_F_PM[k]; //122:distance to following PM is greater or equal to 1     136:distance to following PM is greater or equal to 15
            j++;
        }
        for(k=0;k<Left_POS_Window_Size;k++)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = L_POS[l][k];
                j++;
            }
        }
        for(k=0;k<Left_POS_Window_Size;k++)
        {
            for(l=0;l<4;l++)
            {
                LinFea[i][j] = L_WL[l][k];
                j++;
            }
        }
        for(k=0;k<Right_POS_Window_Size;k++)
        {
            for(l=0;l<96;l++)
            {
                LinFea[i][j] = R_POS[l][k];
                j++;
            }
        }
        for(k=0;k<Right_POS_Window_Size;k++)
        {
            for(l=0;l<4;l++)
            {
                LinFea[i][j] = R_WL[l][k];
                j++;
            }
        }

        //for(k=0;k<d;k++)
        //	printf("%d ",LinFea[i][k]);
        //printf("\n");



    }// end of for(i=0 ; i<(p_data->DataNum) ; i++){
    //////////////////////////////////////////////////////////////////////////////

    /////  find leaf node tag from break-acoustic model ///////////////////////////
    Tags = (int *) calloc(p_data->DataNum, sizeof(int));

    M = sizeof(Q_B_L1)/sizeof(Q_B_L1[0][0]);
    N = sizeof(Q_B_L1[0])/sizeof(Q_B_L1[0][0]); // node #
    M = M/N;                                      // Question #
    //printf("M:%d, N:%d\n",M,N);

    //for(i=0 ; i<1 ; i++){
    for(i=0 ; i<((p_data->DataNum)-1) ; i++) //Tags[N-1] = 0
    {
        tag = 0;
        max_depth = 0;
        for(n=0;n<N;n++)
        {
            depth = 0;
            if(B_L1[n] == (p_data->prosodys[i].B))
            {
                for(m=0;m<M;m++)
                {
                    depth++;
                    if(Q_B_L1[m][n] == 0)
                        break;
                    if( (Q_B_L1[m][n] > 0) && (LinFea[i][abs(Q_B_L1[m][n])-1]==0)  )
                        break;
                    if( (Q_B_L1[m][n] < 0) && (LinFea[i][abs(Q_B_L1[m][n])-1]==1)  )
                        break;
                }

            }
            if(depth > max_depth)
            {
                max_depth = depth;
                tag = n;
            }

        }
        Tags[i] = tag + 1;
        //printf("BL1 tag %d: %d \n",(i+1), (tag+1));
    }


    // fins SR-Bin
    M = sizeof(SR_Bin_forPause) / sizeof(SR_Bin_forPause[0]);
    max_dist = fabs(SR - SR_Bin_forPause[0]);
    max_bin_index = 0;
    for(i=1;i<M;i++)
    {
        dist = fabs(SR - SR_Bin_forPause[i]);
        if(dist < max_dist)
        {
            max_dist = dist;
            max_bin_index = i;
        }
    }

    // SR-Pause mapping
    for(i=0 ; i<((p_data->DataNum)-1) ; i++)
    {
        p_data->prosodys[i].pause = SR_Pau[max_bin_index][Tags[i]-1];
        //printf("i:%d, pause=%.4f\n",i+1, p_data->prosodys[i].pause);
    }
    p_data->prosodys[(p_data->DataNum)-1].pause = 1.0;






    free(code411_p_1);
    free(LinFea);
    free(L_POS);
    free(R_POS);
    free(L_WL);
    free(R_WL);
    free(Tags);


}


void LogF0ContourPrediction(ProsodyDataSet *p_data, float SR)
{
    int i = 0, j = 0, k = 0, Pre_B = 0, Fol_B = 0, Tone = 0, Pre_Tone = 0, Fol_Tone = 0;
    float OE[4] = {0.0}, norm_m = 0.0, norm_std = 0.0;


    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        //printf("tone: %d\n",);
        if(i==0) // sent. start
        {
            Tone = p_data->lins[i].tone;
            Fol_Tone = p_data->lins[i+1].tone;
            Pre_Tone = 5;
            Fol_B = p_data->prosodys[i].B;
            Pre_B = BTypeNum + 1 ;

            for(j=0;j<4;j++)
            {
                OE[j] = MuX[j] + PT[Tone-1][j] + PP[(p_data->prosodys[i].PS_p)-1][j] + PCf[j][Pre_Tone-1][Tone-1][Pre_B-1] + PCb[j][Tone-1][Fol_Tone-1][Fol_B-1] ;
            }
        }
        else if(i==((p_data->DataNum)-1)) // sent. end
        {
            Tone = p_data->lins[i].tone;
            Fol_Tone = 5;
            Pre_Tone = p_data->lins[i-1].tone;
            Fol_B = p_data->prosodys[i].B;
            Pre_B = p_data->prosodys[i-1].B;

            for(j=0;j<4;j++)
            {
                OE[j] = MuX[j] + PT[Tone-1][j] + PP[(p_data->prosodys[i].PS_p)-1][j] + PCf[j][Pre_Tone-1][Tone-1][Pre_B-1] + PCb[j][Tone-1][Fol_Tone-1][Fol_B-1] ;
            }

        }
        else
        {
            Tone = p_data->lins[i].tone;
            Fol_Tone = p_data->lins[i+1].tone;
            Pre_Tone = p_data->lins[i-1].tone;
            Fol_B = p_data->prosodys[i].B;
            Pre_B = p_data->prosodys[i-1].B;

            for(j=0;j<4;j++)
            {
                OE[j] = MuX[j] + PT[Tone-1][j] + PP[(p_data->prosodys[i].PS_p)-1][j] + PCf[j][Pre_Tone-1][Tone-1][Pre_B-1] + PCb[j][Tone-1][Fol_Tone-1][Fol_B-1] ;
            }
        }
        //printf("pre_tone:%d tone:%d      pre_B:%d  PCf:%.4f\n",Pre_Tone,Tone,Pre_B,PCf[0][Pre_Tone-1][Tone-1][Pre_B-1]);
        //printf("Tone:%d,    fol_Tone:%d, Fol_B:%d, PCb:%.4f \n",Tone, Fol_Tone,Fol_B, PCb[0][Tone-1][Fol_Tone-1][Fol_B-1]);
        //printf("PT:%.4f PP:%.4f MuX:%.4f\n",MuX[0],PT[Tone-1][0],PP[(p_data->prosodys[i].PS_p)-1][0]);

        // denormalize
        //printf("%d: ",i+1);
        for(j=0;j<4;j++)
        {
            norm_m = SR * norm_SR_lf0_m_P[0][Tone-1][j] + norm_SR_lf0_m_P[1][Tone-1][j];
            norm_std = SR * norm_SR_lf0_std_P[0][Tone-1][j] + norm_SR_lf0_std_P[1][Tone-1][j];
            OE[j] = ( OE[j] - norm_global_lf0_m[Tone-1][j] ) /  norm_global_lf0_std[Tone-1][j] * norm_std + norm_m;
            p_data->prosodys[i].lf0_OE[j] = OE[j];
            //printf("%.4f ",OE[j]);
        }
        //printf("\n");
    }





}


void DurationPrediction(ProsodyDataSet *p_data, float SR)
{
    int i = 0, j = 0, k = 0, Tone = 0, IF = 0;
    float dur = 0.0, norm_m = 0.0, norm_std = 0.0;


    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        IF = code411_to_BaseSyllable[(p_data->lins[i].code411)-1];
        Tone = p_data->lins[i].tone;

        dur = MuD + DT[Tone-1] + DP[(p_data->prosodys[i].PS_q)-1] + DIF[IF-1] ;


        // denormalize
        //printf("%d: ",i+1);
        norm_m = (SR * norm_SR_dur_m_P[0]) + norm_SR_dur_m_P[1];
        norm_std = (SR * SR * norm_SR_dur_std_P[0]) + (SR * norm_SR_dur_std_P[1]) + norm_SR_dur_std_P[2];
        dur = ( dur - norm_global_dur_m ) /  norm_global_dur_std * norm_std + norm_m;
        if( dur < 0.05 )
            dur = 0.05;
        p_data->prosodys[i].duration = dur;
        //printf("%.4f ",dur);
        //printf("\n");
    }


}




void EnergyLevelPrediction(ProsodyDataSet *p_data, float SR)
{
    int i = 0, j = 0, k = 0, Tone = 0, IF = 0;
    float eng = 0.0, norm_m = 0.0, norm_std = 0.0;


    for(i=0 ; i<(p_data->DataNum) ; i++)
    {
        IF = code411_to_final[(p_data->lins[i].code411)-1];
        Tone = p_data->lins[i].tone;

        eng = MuE + ET[Tone-1] + EP[(p_data->prosodys[i].PS_r)-1] + EIF[IF-1] ;


        // denormalize
        //printf("%d: ",i+1);
        p_data->prosodys[i].energy_level = eng;
        //printf("%.4f ",eng);
        //printf("\n");
    }

}


#ifdef FULLCONTEXT
void GenHTKLabel(ProsodyDataSet *p_data)
{
	int i = 0, j = 0;
	long ofs = 0;
	float tunit = 10000000.0;
	long st = 0, et = 0;
	float sildur = 0.1;
	int code;
	p_data->htklab = (HTKLabStream *) calloc(1, sizeof(HTKLabStream));
	p_data->htklab->size = p_data->DataNum * 3 /*3 for initial+final+sp*/ + 2 /* 2 for silences of sentence start and end*/;
	p_data->htklab->items = (HTK_label_item *) calloc(p_data->htklab->size, sizeof(HTK_label_item));

	// first silence
	st = 0;
	et = ((long) (sildur * tunit) );
	p_data->htklab->items[0].begin_t = st;
	p_data->htklab->items[0].end_t = et;
	strcpy(p_data->htklab->items[0].inifin, "sil");
	strcpy(p_data->htklab->items[0].syllable, "");
	j = 1;
	ofs += et;
	for( i = 0; i < p_data->DataNum; i ++ ) {
		p_data->htklab->items[j].begin_t = ofs;
		p_data->htklab->items[j].end_t = ofs;
		code = (p_data->lins[i].code411) % 1000;
		strcpy(p_data->htklab->items[j].inifin, code411_to_initial_char[code]);
		strcpy(p_data->htklab->items[j].syllable, code411_to_syllable_char[code]);
		j ++;
		p_data->htklab->items[j].begin_t = ofs;
		ofs += ( (long) (p_data->prosodys[i].duration * tunit) );
		p_data->htklab->items[j].end_t = ofs;
		strcpy(p_data->htklab->items[j].inifin, code411_to_final_char[code]);
		strcpy(p_data->htklab->items[j].syllable, "");
		j ++;
		p_data->htklab->items[j].begin_t = ofs;
		ofs += ( (long) (p_data->prosodys[i].pause * tunit) );
		p_data->htklab->items[j].end_t = ofs;
		strcpy(p_data->htklab->items[j].inifin, "sp");
		strcpy(p_data->htklab->items[j].syllable, "");
		j ++;
	}
	p_data->htklab->items[j].begin_t = ofs;
	ofs += ((long) (sildur * tunit) );
	p_data->htklab->items[j].end_t = ofs;
	strcpy(p_data->htklab->items[j].inifin, "sil");
	strcpy(p_data->htklab->items[j].syllable, "");
}


int SaveHTKLabelFn(ProsodyDataSet *p_data, char *fn)
{
	int i = 0;
	FILE *fp = fopen(fn, "w");
	if( !fp ) {
		fprintf(stderr, "Cannot save %s\n", fn);
		return 0;
	}
	for( i = 0; i < p_data->htklab->size; i ++ ) {
		fprintf(fp, "%d %d %s", p_data->htklab->items[i].begin_t,
			p_data->htklab->items[i].end_t,
			p_data->htklab->items[i].inifin);
		if( p_data->htklab->items[i].syllable[0] != '\0' ) {
			fprintf(fp, " %s\n", p_data->htklab->items[i].syllable);
		}
		else {
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
}


HTS_lab_item *HTS_lab_item_new(void)
{
	HTS_lab_item *p = (HTS_lab_item *) calloc(1, sizeof(HTS_lab_item));
	strcpy(p->CH, "");
	p->code = 0;
	p->index = 0;
	p->pos = 0;
	p->subindex = 0;
	p->subpos = 0;
	p->next = NULL;
	p->previous = NULL;

	p->lab_start = 0;
	p->lab_end = 0;


	// phone level
	// initial-final level
	strcpy(p->pre_inifin, "");
	strcpy(p->cur_inifin, "");
	strcpy(p->fol_inifin, "");
	p->inifin_in_syl = 0;
	// syllable level
	p->pre_tone = 0;
	p->cur_tone = 0;
	p->fol_tone = 0;
	p->F_syl_in_subword[0] = p->F_syl_in_subword[1] = 0;
	p->B_syl_in_subword[0] = p->B_syl_in_subword[1] = 0;
	p->F_syl_in_word[0] = p->F_syl_in_word[1] = 0;
	p->B_syl_in_word[0] = p->B_syl_in_word[1] = 0;
	// word level
	strcpy(p->pre_PM, "x");
	strcpy(p->fol_PM, "x");
	p->pre_PM_id = 0;
	p->fol_PM_id = 0;
	strcpy(p->pre_3_POS, "x");
	strcpy(p->pre_2_POS, "x");
	strcpy(p->pre_1_POS, "x");
	strcpy(p->cur_POS, "x");
	strcpy(p->fol_1_POS, "x");
	strcpy(p->fol_2_POS, "x");
	strcpy(p->fol_3_POS, "x");
	strcpy(p->pre_3_SubPOS, "x");
	strcpy(p->pre_2_SubPOS, "x");
	strcpy(p->pre_1_SubPOS, "x");
	strcpy(p->cur_SubPOS, "x");
	strcpy(p->fol_1_SubPOS, "x");
	strcpy(p->fol_2_SubPOS, "x");
	strcpy(p->fol_3_SubPOS, "x");
	p->pre_3_WL = 0;
	p->pre_2_WL = 0;
	p->pre_1_WL = 0;
	p->cur_WL = 0;
	p->fol_1_WL =0;
	p->fol_2_WL = 0;
	p->fol_3_WL = 0;
	p->pre_3_SWL = 0;
	p->pre_2_SWL = 0;
	p->pre_1_SWL = 0;
	p->cur_SWL = 0;
	p->fol_1_SWL =0;
	p->fol_2_SWL = 0;
	p->fol_3_SWL = 0;
	p->pre_w_cross[0] = 0;
	p->pre_w_cross[1] = 0;
	p->pre_w_cross[2] = 0;
	p->fol_w_cross[0] = 0;
	p->fol_w_cross[1] = 0;
	p->fol_w_cross[2] = 0;
	p->pre_sw_cross[0] = 0;
	p->pre_sw_cross[1] = 0;
	p->pre_sw_cross[2] = 0;
	p->fol_sw_cross[0] = 0;
	p->fol_sw_cross[1] = 0;
	p->fol_sw_cross[2] = 0;
	// phrase level
	strcpy(p->pre_1_Ph, "x");
	strcpy(p->fol_1_Ph, "x");
	p->pre_1_PhL = 0;
	p->fol_1_PhL = 0;
	p->F_syl_in_phrase[0] = p->F_syl_in_phrase[1] = 0;
	p->B_syl_in_phrase[0] = p->B_syl_in_phrase[1] = 0;
	// sentence level
	p->F_syl_in_sent[0] = p->F_syl_in_sent[1] = 0;
	p->B_syl_in_sent[0] = p->B_syl_in_sent[1] = 0;
	p->sent_length_in_syl = 0;
	p->pre_sent_length_in_syl = 0;
	p->fol_sent_length_in_syl = 0;
	return p;
}

void HTS_lab_item_free(HTS_lab_item *p)
{
	if( p )
		free(p);
}

HTS_lab *HTS_lab_new(void)
{
	HTS_lab *p = (HTS_lab *) calloc(1, sizeof(HTS_lab));
	p->num_of_HTS_lab_item = 0;
	p->items_head = NULL;
	return p;
}

void HTS_lab_free(HTS_lab *p) {
	HTS_lab_item *tmp, *tmp1;
	tmp = p->items_head;
	while( tmp->next != NULL )
	{
		tmp1 = tmp;
		tmp = tmp->next;
		HTS_lab_item_free(tmp1);
	}
}

void HTS_lab_Add_item(HTS_lab *p, HTS_lab_item *items)
{
	HTS_lab_item *tmp, *tmp1;
	tmp = p->items_head;
	if( tmp == NULL )
	{
		p->items_head = items;
	}
	else
	{
		while( tmp->next != NULL )
		{
			tmp = tmp->next;
		}
		tmp->next = items;
		items->previous = tmp;
	}
}

HTS_lab_item *HTS_lab_Find_HTS_item_tail(HTS_lab *p)
{
	HTS_lab_item *tmp;
	tmp = p->items_head;
	if( tmp == NULL )
	{
		return NULL;
	}
	while( tmp->next != NULL )
	{
		tmp = tmp->next;
	}
	return tmp;
}


int HTS_lab_Find_lab_acording_to_transx(TRANSStream *transx, int ith_syl, HTKLabStream *lab, int *start_in_lab, int *end_in_lab)
{
	TRANS *trs_item = &(transx->items[ith_syl]);
	int i = 0;
	int syl_index = 0;
	while(i<lab->size)
	{
		//printf("fuck0 %s\n",lab.items[i]->syllable);
		if( strcmp(lab->items[i].syllable,"") != 0 )
		{
			//printf("fuck1\n");
			if( syl_index == trs_item->syl_idx )
			{
				//printf("fuck2\n");
				*start_in_lab = i;
				i ++;
				break;
			}
			syl_index ++;
		}
		i ++;
	}
	if(i>lab->size)
	{
		printf("Cannot find correspondence between transx and lab files in trans file %s, line %d\n", transx->filename, ith_syl+1);
		return 0;
	}
	while(i<lab->size)
	{
		if( (i>0 && strcmp(lab->items[i].inifin, "sil") ==0) || strcmp(lab->items[i].syllable, "") != 0 )
		{
			*end_in_lab = i-1;
			break;
		}
		i ++;
	}
	if(i>lab->size)
	{
		printf("Cannot find correspondence between transx and lab files in trans file %s, line %d\n", transx->filename, ith_syl+1);
		return 0;
	}
	if( (trs_item->code%1000) < (MaxSylNum+1) )
	{
		//printf("%d ,%d,%d   %d\n", (trs_item->code%1000), start_in_lab,syl_index, lab.num_of_HTK_lab_item );
		//printf("transx:%s , lab:%s\n",code411_to_pinyin[(trs_item->code%1000)], lab.items[start_in_lab]->syllable);
		if( strcmp(code411_to_syllable_char[(trs_item->code%1000)], lab->items[*start_in_lab].syllable) != NULL)
		{
			printf("Phonetic transcription mismatch between line %d of %s and line %d of %s\n", ith_syl+1, "transx", start_in_lab+1, "HTK label");
			return -1;
		}
	}
	else
	{
		printf("error code 411 in transx file %s, line %d\n", transx->filename, ith_syl+1);
		return -2;
	}




	return 1;
}


int MakeHTSlab(HTS_lab *p, TRANSStream *trans, HTKLabStream *lab, double pau_thr)
{
	int i, j;
	HTS_lab_item *item;
	HTS_lab_item *pre_item;
	int start_in_lab = -1, end_in_lab = -1;
	int pre_syl_idx, fol_syl_idx;
	int pre_tone, cur_tone, fol_tone;
	int syl_in_Ph_F[2] = {0,0}, syl_in_Ph_B[2] = {0,0};
	int syl_in_LW_F[2] = {0,0}, syl_in_LW_B[2] = {0,0};
	int syl_in_SLW_F[2] = {0,0}, syl_in_SLW_B[2] = {0,0};
	char pre_PM[64], fol_PM[64];
	int pre_PM_id, fol_PM_id;
	int pre_word_idx, pre_pre_word_idx, pre_pre_pre_word_idx, fol_word_idx, fol_fol_word_idx, fol_fol_fol_word_idx;
	int pre_subword_idx, pre_pre_subword_idx, pre_pre_pre_subword_idx, fol_subword_idx, fol_fol_subword_idx, fol_fol_fol_subword_idx;
	int pre_3_pos, pre_2_pos, pre_1_pos, fol_1_pos, fol_2_pos, fol_3_pos, cur_pos;
	int pre_3_WL, pre_2_WL, pre_1_WL, fol_1_WL, fol_2_WL, fol_3_WL, cur_WL;
	int pre_3_subpos, pre_2_subpos, pre_1_subpos, fol_1_subpos, fol_2_subpos, fol_3_subpos, cur_subpos;
	int pre_3_SWL, pre_2_SWL, pre_1_SWL, fol_1_SWL, fol_2_SWL, fol_3_SWL, cur_SWL;
	int pre_1_phtype, fol_1_phtype=0;
	int pre_1_phl, fol_1_phl=0;
	int sent_start_idx, sent_end_idx, pre_sent_start_idx, pre_sent_end_idx, fol_sent_start_idx, fol_sent_end_idx;
	int sent_len, pre_sent_len, fol_sent_len;
	int fol_ph_index;
	int F_syl_in_sent[2] = {0,0}, B_syl_in_sent[2] = {0,0};
	int pre_w_cross[3];
	int fol_w_cross[3];
	int pre_sw_cross[3];
	int fol_sw_cross[3];
	int pre_ph_cross;
	int fol_ph_cross;
	double dur;

	for(i=0;i<trans->size;i++)
	{

		if( trans->items[i].is_PM == true )//ignore PM
		{
		}
		else
		{
			// find previous syllable
			pre_syl_idx = -1;
			for(j=i-1;j>=0;j--)
			{
				if( trans->items[j].syl_idx >= 0 )
				{
					pre_syl_idx = j;
					break;
				}
			}

			// find following syllable
			fol_syl_idx = -1;
			for(j=i+1;j<trans->size;j++)
			{
				if( trans->items[j].syl_idx >= 0 )
				{
					fol_syl_idx = j;
					break;
				}
			}

			// find previous tone
			if( pre_syl_idx >= 0)
			{
				pre_tone = (int)floor((double)(trans->items[pre_syl_idx].code)/1000);
			}
			else
			{
				pre_tone = 0;
			}
			// find following tone
			if( fol_syl_idx >= 0 && fol_syl_idx < trans->size)
			{
				fol_tone = (int)floor((double)(trans->items[fol_syl_idx].code)/1000);
			}
			else
			{
				fol_tone = 0;
			}
			// find current tone
			cur_tone = (int)floor((double)(trans->items[i].code)/1000);




			// find syllable position in a subword forward
			syl_in_SLW_F[0] = trans->items[i].subindex%100;
			syl_in_SLW_F[1] = (int)floor((double)(trans->items[i].subindex)/100);
			// find syllable position in a subword backward
			syl_in_SLW_B[0] = (int)floor((double)(trans->items[i].subindex)/100) - trans->items[i].subindex%100 + 1;
			syl_in_SLW_B[1] = (int)floor((double)(trans->items[i].subindex)/100);

			// find syllable position in a word forward
			syl_in_LW_F[0] = trans->items[i].index%100;
			syl_in_LW_F[1] = (int)floor((double)(trans->items[i].index)/100);
			// find syllable position in a word backward
			syl_in_LW_B[0] = (int)floor((double)(trans->items[i].index)/100) - trans->items[i].index%100 + 1;
			syl_in_LW_B[1] = (int)floor((double)(trans->items[i].index)/100);




			// find previous PM
			if( i>0 )
			{
				if( trans->items[i-1].is_PM )
				{
					strcpy(pre_PM, trans->items[i-1].syl);

					pre_PM_id = trans->items[i-1].PM_id;
				}
				else
				{
					strcpy(pre_PM, "x");
					pre_PM_id = 0;
				}
			}
			else
			{
				strcpy(pre_PM, "x");
				pre_PM_id = 0;
			}
			// find following PM
			if( i<(trans->size-1) )
			{
				if( trans->items[i+1].is_PM )
				{
					strcpy(fol_PM, trans->items[i+1].syl);
					//printf("%d\n",trans->items[i+1]->PM_id);  ///cchang test
					fol_PM_id = trans->items[i+1].PM_id;
				}
				else
				{
					strcpy(fol_PM, "x");
					fol_PM_id = 0;
				}
			}
			else
			{
				strcpy(fol_PM, "x");
				fol_PM_id = 0;
			}

















			// find previous sub-word idx
			pre_sw_cross[0] = 0;
			pre_sw_cross[1] = 0;
			pre_sw_cross[2] = 0;
			pre_subword_idx = i- abs((int)(trans->items[i].subindex%100));
			if( pre_subword_idx > 0 )
			{
				while( pre_subword_idx>0 && trans->items[pre_subword_idx].is_PM )
				{
					pre_sw_cross[0] = 1;
					pre_subword_idx = pre_subword_idx - 1;
				}
				pre_subword_idx = pre_subword_idx - (int)floor((double)(trans->items[pre_subword_idx].subindex/100)) + 1;
			}
			pre_pre_subword_idx = -1;
			if( pre_subword_idx > 0 )
			{
				pre_pre_subword_idx = pre_subword_idx - 1;
				if( pre_pre_subword_idx > 0)
				{
					while( pre_pre_subword_idx>0 && trans->items[pre_pre_subword_idx].is_PM )
					{
						pre_sw_cross[1] = 1;
						pre_pre_subword_idx = pre_pre_subword_idx - 1;
					}
					pre_pre_subword_idx = pre_pre_subword_idx - (int)floor((double)(trans->items[pre_pre_subword_idx].subindex)/100) + 1;
				}
			}
			pre_pre_pre_subword_idx = -1;
			if( pre_pre_subword_idx > 0 )
			{
				pre_pre_pre_subword_idx = pre_pre_subword_idx - 1;
				if( pre_pre_pre_subword_idx > 0)
				{
					while( pre_pre_pre_subword_idx>0 && trans->items[pre_pre_pre_subword_idx].is_PM )
					{
						pre_sw_cross[2] = 1;
						pre_pre_pre_subword_idx = pre_pre_pre_subword_idx - 1;
					}
					pre_pre_pre_subword_idx = pre_pre_pre_subword_idx - (int)floor((double)(trans->items[pre_pre_pre_subword_idx].subindex)/100) + 1;
				}
			}
			if( pre_subword_idx >=0 )
			{
				pre_1_subpos = trans->items[pre_subword_idx].subpos;
				pre_1_SWL = (int)floor((double)(trans->items[pre_subword_idx].subindex)/100);
			}
			else
			{
				pre_1_subpos = 0;
				pre_1_SWL = 0;
			}
			if( pre_pre_subword_idx >=0 )
			{
				pre_2_subpos = trans->items[pre_pre_subword_idx].subpos;
				pre_2_SWL = (int)floor((double)(trans->items[pre_pre_subword_idx].subindex)/100);
			}
			else
			{
				pre_2_subpos = 0;
				pre_2_SWL = 0;
			}
			if( pre_pre_pre_subword_idx >=0 )
			{
				pre_3_subpos = trans->items[pre_pre_pre_subword_idx].subpos;
				pre_3_SWL = (int)floor((double)(trans->items[pre_pre_pre_subword_idx].subindex)/100);
			}
			else
			{
				pre_3_subpos = 0;
				pre_3_SWL = 0;
			}


			// find previous word idx
			pre_w_cross[0] = 0;
			pre_w_cross[1] = 0;
			pre_w_cross[2] = 0;
			pre_word_idx = i- abs((int)(trans->items[i].index%100));
			if( pre_word_idx > 0 )
			{
				while( pre_word_idx>0 && trans->items[pre_word_idx].is_PM )
				{
					pre_w_cross[0] = 1;
					pre_word_idx = pre_word_idx - 1;
				}
				pre_word_idx = pre_word_idx - (int)floor((double)(trans->items[pre_word_idx].index/100)) + 1;
			}
			pre_pre_word_idx = -1;
			if( pre_word_idx > 0 )
			{
				pre_pre_word_idx = pre_word_idx - 1;
				if( pre_pre_word_idx > 0)
				{
					while( pre_pre_word_idx>0 && trans->items[pre_pre_word_idx].is_PM )
					{
						pre_w_cross[1] = 1;
						pre_pre_word_idx = pre_pre_word_idx - 1;
					}
					pre_pre_word_idx = pre_pre_word_idx - (int)floor((double)(trans->items[pre_pre_word_idx].index)/100) + 1;
				}
			}
			pre_pre_pre_word_idx = -1;
			if( pre_pre_word_idx > 0 )
			{
				pre_pre_pre_word_idx = pre_pre_word_idx - 1;
				if( pre_pre_pre_word_idx > 0)
				{
					while( pre_pre_pre_word_idx>0 && trans->items[pre_pre_pre_word_idx].is_PM )
					{
						pre_w_cross[2] = 1;
						pre_pre_pre_word_idx = pre_pre_pre_word_idx - 1;
					}
					pre_pre_pre_word_idx = pre_pre_pre_word_idx - (int)floor((double)(trans->items[pre_pre_pre_word_idx].index)/100) + 1;
				}
			}
			if( pre_word_idx >=0 )
			{
				pre_1_pos = trans->items[pre_word_idx].pos;
				pre_1_WL = (int)floor((double)(trans->items[pre_word_idx].index)/100);
			}
			else
			{
				pre_1_pos = 0;
				pre_1_WL = 0;
			}
			if( pre_pre_word_idx >=0 )
			{
				pre_2_pos = trans->items[pre_pre_word_idx].pos;
				pre_2_WL = (int)floor((double)(trans->items[pre_pre_word_idx].index)/100);
			}
			else
			{
				pre_2_pos = 0;
				pre_2_WL = 0;
			}
			if( pre_pre_pre_word_idx >=0 )
			{
				pre_3_pos = trans->items[pre_pre_pre_word_idx].pos;
				pre_3_WL = (int)floor((double)(trans->items[pre_pre_pre_word_idx].index)/100);
			}
			else
			{
				pre_3_pos = 0;
				pre_3_WL = 0;
			}







			// find following subword idx
			fol_sw_cross[0] = 0;
			fol_sw_cross[1] = 0;
			fol_sw_cross[2] = 0;
			fol_fol_subword_idx = trans->size;
			fol_subword_idx = i - (trans->items[i].subindex%100) + 1 + (int)floor((double)(trans->items[i].subindex/100));

			if( fol_subword_idx < trans->size )
			{
				while( fol_subword_idx<trans->size && trans->items[fol_subword_idx].is_PM )
				{
					fol_sw_cross[0] = 1;
					fol_subword_idx ++;
				}
				if( fol_subword_idx < trans->size )
				{
					fol_fol_subword_idx = fol_subword_idx +(int)floor((double)(trans->items[fol_subword_idx].subindex/100));
					while( fol_fol_subword_idx < trans->size && trans->items[fol_fol_subword_idx].is_PM )
					{
						fol_sw_cross[1] = 1;
						fol_fol_subword_idx ++;
					}
					if( fol_fol_subword_idx < trans->size )
					{
						fol_fol_fol_subword_idx = fol_fol_subword_idx +(int)floor((double)(trans->items[fol_fol_subword_idx].subindex/100));
						while( fol_fol_fol_subword_idx < trans->size && trans->items[fol_fol_fol_subword_idx].is_PM )
						{
							fol_sw_cross[2] = 1;
							fol_fol_fol_subword_idx ++;
						}
					}
				}
			}
			if( fol_subword_idx < trans->size )
			{
				fol_1_subpos = trans->items[fol_subword_idx].subpos;
				fol_1_SWL = (int)floor((double)(trans->items[fol_subword_idx].subindex)/100);
			}
			else
			{
				fol_1_subpos = 0;
				fol_1_SWL = 0;
			}
			if( fol_fol_subword_idx < trans->size )
			{
				fol_2_subpos = trans->items[fol_fol_subword_idx].subpos;
				fol_2_SWL = (int)floor((double)(trans->items[fol_fol_subword_idx].subindex)/100);
			}
			else
			{
				fol_2_subpos = 0;
				fol_2_SWL = 0;
			}
			if( fol_fol_fol_subword_idx < trans->size )
			{
				fol_3_subpos = trans->items[fol_fol_fol_subword_idx].subpos;
				fol_3_SWL = (int)floor((double)(trans->items[fol_fol_fol_subword_idx].subindex)/100);
			}
			else
			{
				fol_3_subpos = 0;
				fol_3_SWL = 0;
			}





			// find following word idx
			fol_w_cross[0] = 0;
			fol_w_cross[1] = 0;
			fol_w_cross[2] = 0;
			fol_fol_word_idx = trans->size;
			fol_word_idx = i - (trans->items[i].index%100) + 1 + (int)floor((double)(trans->items[i].index/100));

			if( fol_word_idx < trans->size )
			{
				while( fol_word_idx<trans->size && trans->items[fol_word_idx].is_PM )
				{
					fol_w_cross[0] = 1;
					fol_word_idx ++;
				}
				if( fol_word_idx < trans->size )
				{
					fol_fol_word_idx = fol_word_idx +(int)floor((double)(trans->items[fol_word_idx].index/100));
					while( fol_fol_word_idx < trans->size && trans->items[fol_fol_word_idx].is_PM )
					{
						fol_w_cross[1] = 1;
						fol_fol_word_idx ++;
					}
					if( fol_fol_word_idx < trans->size )
					{
						fol_fol_fol_word_idx = fol_fol_word_idx +(int)floor((double)(trans->items[fol_fol_word_idx].index/100));
						while( fol_fol_fol_word_idx < trans->size && trans->items[fol_fol_fol_word_idx].is_PM )
						{
							fol_w_cross[2] = 1;
							fol_fol_fol_word_idx ++;
						}
					}
				}
			}
			if( fol_word_idx < trans->size )
			{
				fol_1_pos = trans->items[fol_word_idx].pos;
				fol_1_WL = (int)floor((double)(trans->items[fol_word_idx].index)/100);
			}
			else
			{
				fol_1_pos = 0;
				fol_1_WL = 0;
			}
			if( fol_fol_word_idx < trans->size )
			{
				fol_2_pos = trans->items[fol_fol_word_idx].pos;
				fol_2_WL = (int)floor((double)(trans->items[fol_fol_word_idx].index)/100);
			}
			else
			{
				fol_2_pos = 0;
				fol_2_WL = 0;
			}
			if( fol_fol_fol_word_idx < trans->size )
			{
				fol_3_pos = trans->items[fol_fol_fol_word_idx].pos;
				fol_3_WL = (int)floor((double)(trans->items[fol_fol_fol_word_idx].index)/100);
			}
			else
			{
				fol_3_pos = 0;
				fol_3_WL = 0;
			}




			// find current LW/SLW/Ph
			cur_subpos = trans->items[i].subpos;
			cur_SWL = (int)floor((double)(trans->items[i].subindex)/100);
			cur_pos = trans->items[i].pos;
			cur_WL = (int)floor((double)(trans->items[i].index)/100);
			//pre_1_phtype = trans->items[i].phtype;
			//pre_1_phl = (int)floor((double)(trans->items[i].phindex)/100);
			if( HTS_lab_Find_lab_acording_to_transx(trans, i, lab, &start_in_lab, &end_in_lab) != 1 )
			{
				return false;
			}



			// find sentance start syllable index
			sent_start_idx = 0;
			for(j=i-1;j>=0;j--)
			{
				if( trans->items[j].is_PM )
				{
					sent_start_idx = j + 1;
					break;
				}
			}

			// find previous sentence end syllable index
			pre_sent_end_idx = sent_start_idx - 1;
			for(j=pre_sent_end_idx;j>=0;j--)
			{
				if( !trans->items[j].is_PM )
				{
					pre_sent_end_idx = j;
					break;
				}
			}


			// find previous sentence start syllable index
			pre_sent_start_idx = pre_sent_end_idx;
			for(j=pre_sent_start_idx;j>=0;j--)
			{
				if( trans->items[j].is_PM )
				{
					pre_sent_start_idx = j+1;
					break;
				}
				if( j==0 )
				{
					pre_sent_start_idx = 0;
					break;
				}
			}

			// find sentence end syllable index
			sent_end_idx = trans->size-1;

			for(j=i+1;(j<trans->size);j++)
			{
				if( trans->items[j].is_PM )
				{
					sent_end_idx = j-1;
					break;
				}
			}

			// find following sentence start syllable index
			fol_sent_start_idx = sent_end_idx + 1; //cchang:有改(ok)

			for(j=fol_sent_start_idx;(j<trans->size);j++)//有改(ok)
			{
				if( !trans->items[j].is_PM )
				{
					fol_sent_start_idx = j;
					break;
				}



			}

			// find following sentence end syllable index
			fol_sent_end_idx = fol_sent_start_idx;
			for(j=fol_sent_end_idx;(j<trans->size);j++)//cchang:大大有問題(ok)
			{
				if( trans->items[j].is_PM )
				{

					fol_sent_end_idx = j-1;

					break;
				}


			}



			// find length of the current sentence
			sent_len = sent_end_idx - sent_start_idx + 1;
			if( pre_sent_end_idx < 0 )
			{
				pre_sent_len = 0;
			}
			else
			{
				pre_sent_len = pre_sent_end_idx - pre_sent_start_idx + 1;

			}

			fol_sent_len = fol_sent_end_idx - fol_sent_start_idx + 1;

			F_syl_in_sent[0] = i - sent_start_idx + 1;
			F_syl_in_sent[1] = sent_len;
			B_syl_in_sent[0] = sent_end_idx -i + 1;
			B_syl_in_sent[1] = sent_len;

			pre_item = HTS_lab_Find_HTS_item_tail(p);
			for(j=start_in_lab;j<=end_in_lab;j++)
			{
				dur = ((double)(lab->items[j].end_t - lab->items[j].begin_t))/10000000;
				if( dur < 0 )
				{
					printf("Error in %s: %s, line %d, label: %s:end time is advanced to start time\n", "HTK label", trans->filename, j+1, lab->items[j].inifin);
					exit(1);
				}
				if( strcmp(lab->items[j].inifin, "sp") == 0 )
				{
					if( dur > pau_thr )
					{
						item = HTS_lab_item_new();
						strcpy(item->cur_inifin, lab->items[j].inifin);
						item->lab_start = lab->items[j].begin_t;
						item->lab_end = lab->items[j].end_t;

						item->inifin_in_syl = 0;
						item->pre_tone = 0;
						item->cur_tone = 0;
						item->fol_tone = 0;
						item->F_syl_in_subword[0] = syl_in_SLW_F[0];
						item->F_syl_in_subword[1] = syl_in_SLW_F[1];
						item->B_syl_in_subword[0] = syl_in_SLW_B[0];
						item->B_syl_in_subword[1] = syl_in_SLW_B[1];

						item->F_syl_in_word[0] = syl_in_LW_F[0];
						item->F_syl_in_word[1] = syl_in_LW_F[1];
						item->B_syl_in_word[0] = syl_in_LW_B[0];
						item->B_syl_in_word[1] = syl_in_LW_B[1];

						strcpy(item->pre_PM, pre_PM);
						strcpy(item->fol_PM, fol_PM);
						item->pre_PM_id = pre_PM_id;
						item->fol_PM_id = fol_PM_id;


						item->pre_sw_cross[0] = pre_sw_cross[0];
						item->pre_sw_cross[1] = pre_sw_cross[1];
						item->pre_sw_cross[2] = pre_sw_cross[2];
						item->fol_sw_cross[0] = fol_sw_cross[0];
						item->fol_sw_cross[1] = fol_sw_cross[1];
						item->fol_sw_cross[2] = fol_sw_cross[2];


						item->pre_w_cross[0] = pre_w_cross[0];
						item->pre_w_cross[1] = pre_w_cross[1];
						item->pre_w_cross[2] = pre_w_cross[2];
						item->fol_w_cross[0] = fol_w_cross[0];
						item->fol_w_cross[1] = fol_w_cross[1];
						item->fol_w_cross[2] = fol_w_cross[2];


						strcpy(item->pre_1_SubPOS, POSMap[pre_1_subpos]);
						strcpy(item->pre_2_SubPOS, POSMap[pre_2_subpos]);
						strcpy(item->pre_3_SubPOS, POSMap[pre_3_subpos]);
						strcpy(item->cur_SubPOS, POSMap[cur_subpos]);
						strcpy(item->fol_1_SubPOS, POSMap[fol_1_subpos]);
						strcpy(item->fol_2_SubPOS, POSMap[fol_2_subpos]);
						strcpy(item->fol_3_SubPOS, POSMap[fol_3_subpos]);

						strcpy(item->pre_1_POS, POSMap[pre_1_pos]);
						strcpy(item->pre_2_POS, POSMap[pre_2_pos]);
						strcpy(item->pre_3_POS, POSMap[pre_3_pos]);
						strcpy(item->cur_POS, POSMap[cur_pos]);
						strcpy(item->fol_1_POS, POSMap[fol_1_pos]);
						strcpy(item->fol_2_POS, POSMap[fol_2_pos]);
						strcpy(item->fol_3_POS, POSMap[fol_3_pos]);


						item->pre_3_SWL = pre_3_SWL;
						item->pre_2_SWL = pre_2_SWL;
						item->pre_1_SWL = pre_1_SWL;
						item->cur_SWL = cur_SWL;
						item->fol_1_SWL = fol_1_SWL;
						item->fol_2_SWL = fol_2_SWL;
						item->fol_3_SWL = fol_3_SWL;



						item->pre_3_WL = pre_3_WL;
						item->pre_2_WL = pre_2_WL;
						item->pre_1_WL = pre_1_WL;
						item->cur_WL = cur_WL;
						item->fol_1_WL = fol_1_WL;
						item->fol_2_WL = fol_2_WL;
						item->fol_3_WL = fol_3_WL;

//						item->F_syl_in_phrase[0] = syl_in_Ph_F[0];
//						item->F_syl_in_phrase[1] = syl_in_Ph_F[1];
//						item->B_syl_in_phrase[0] = syl_in_Ph_B[0];
//						item->B_syl_in_phrase[1] = syl_in_Ph_B[1];

//						strcpy(item->pre_1_Ph, PhTypeMap[pre_1_phtype]);
//						strcpy(item->fol_1_Ph, PhTypeMap[fol_1_phtype]);
//						item->fol_1_PhL = fol_1_phl;
//						item->pre_1_PhL = pre_1_phl;

						item->F_syl_in_sent[0] = F_syl_in_sent[0];
						item->F_syl_in_sent[1] = F_syl_in_sent[1];
						item->B_syl_in_sent[0] = B_syl_in_sent[0];
						item->B_syl_in_sent[1] = B_syl_in_sent[1];
						item->sent_length_in_syl = sent_len;
						item->pre_sent_length_in_syl = pre_sent_len;
						item->fol_sent_length_in_syl = fol_sent_len;







						if( item->fol_PM_id != 0)
						{



						}
						else
						{
							if( pre_sw_cross[0]==1 )
							{
								item->pre_3_SWL = 0;
								item->pre_2_SWL = 0;
								item->pre_1_SWL = 0;
								strcpy(item->pre_1_SubPOS, POSMap[0]);
								strcpy(item->pre_2_SubPOS, POSMap[0]);
								strcpy(item->pre_3_SubPOS, POSMap[0]);
							}
							if( pre_sw_cross[1]==1 )
							{
								item->pre_3_SWL = 0;
								item->pre_2_SWL = 0;
								strcpy(item->pre_2_SubPOS, POSMap[0]);
								strcpy(item->pre_3_SubPOS, POSMap[0]);
							}
							if( pre_sw_cross[2]==1 )
							{
								item->pre_3_SWL = 0;
								strcpy(item->pre_3_SubPOS, POSMap[0]);
							}
							if( fol_sw_cross[0]==1 )
							{
								item->fol_3_SWL = 0;
								item->fol_2_SWL = 0;
								item->fol_1_SWL = 0;
								strcpy(item->fol_1_SubPOS, POSMap[0]);
								strcpy(item->fol_2_SubPOS, POSMap[0]);
								strcpy(item->fol_3_SubPOS, POSMap[0]);
							}
							if( fol_sw_cross[1]==1 )
							{
								item->fol_3_SWL = 0;
								item->fol_2_SWL = 0;
								strcpy(item->fol_2_SubPOS, POSMap[0]);
								strcpy(item->fol_3_SubPOS, POSMap[0]);
							}
							if( fol_sw_cross[2]==1 )
							{
								item->fol_3_SWL = 0;
								strcpy(item->fol_3_SubPOS, POSMap[0]);
							}




							if( pre_w_cross[0]==1 )
							{
								item->pre_3_WL = 0;
								item->pre_2_WL = 0;
								item->pre_1_WL = 0;
								strcpy(item->pre_1_POS, POSMap[0]);
								strcpy(item->pre_2_POS, POSMap[0]);
								strcpy(item->pre_3_POS, POSMap[0]);
							}
							if( pre_w_cross[1]==1 )
							{
								item->pre_3_WL = 0;
								item->pre_2_WL = 0;
								strcpy(item->pre_2_POS, POSMap[0]);
								strcpy(item->pre_3_POS, POSMap[0]);
							}
							if( pre_w_cross[2]==1 )
							{
								item->pre_3_WL = 0;
								strcpy(item->pre_3_POS, POSMap[0]);
							}
							if( fol_w_cross[0]==1 )
							{
								item->fol_3_WL = 0;
								item->fol_2_WL = 0;
								item->fol_1_WL = 0;
								strcpy(item->fol_1_POS, POSMap[0]);
								strcpy(item->fol_2_POS, POSMap[0]);
								strcpy(item->fol_3_POS, POSMap[0]);
							}
							if( fol_w_cross[1]==1 )
							{
								item->fol_3_WL = 0;
								item->fol_2_WL = 0;
								strcpy(item->fol_2_POS, POSMap[0]);
								strcpy(item->fol_3_POS, POSMap[0]);
							}
							if( fol_w_cross[2]==1 )
							{
								item->fol_3_WL = 0;
								strcpy(item->fol_3_POS, POSMap[0]);
							}

//							if( fol_ph_cross==1 )
//							{
//								strcpy(item->fol_1_Ph, "x");
//								item->fol_1_PhL = 0;
//							}
						}

						HTS_lab_Add_item(p, item);
					}
				}
				else
				{
					item = HTS_lab_item_new();
					strcpy(item->CH, trans->items[i].syl);
					strcpy(item->cur_inifin, lab->items[j].inifin);

					item->lab_start = lab->items[j].begin_t;
					item->lab_end = lab->items[j].end_t;

					item->inifin_in_syl = j - start_in_lab + 1;
					item->pre_tone = pre_tone;
					item->cur_tone = cur_tone;
					item->fol_tone = fol_tone;
					item->F_syl_in_subword[0] = syl_in_SLW_F[0];
					item->F_syl_in_subword[1] = syl_in_SLW_F[1];
					item->B_syl_in_subword[0] = syl_in_SLW_B[0];
					item->B_syl_in_subword[1] = syl_in_SLW_B[1];

					item->F_syl_in_word[0] = syl_in_LW_F[0];
					item->F_syl_in_word[1] = syl_in_LW_F[1];
					item->B_syl_in_word[0] = syl_in_LW_B[0];
					item->B_syl_in_word[1] = syl_in_LW_B[1];
					strcpy(item->pre_PM, pre_PM);
					strcpy(item->fol_PM, fol_PM);
					item->pre_PM_id = pre_PM_id;
					item->fol_PM_id = fol_PM_id;


					item->pre_sw_cross[0] = pre_sw_cross[0];
					item->pre_sw_cross[1] = pre_sw_cross[1];
					item->pre_sw_cross[2] = pre_sw_cross[2];
					item->fol_sw_cross[0] = fol_sw_cross[0];
					item->fol_sw_cross[1] = fol_sw_cross[1];
					item->fol_sw_cross[2] = fol_sw_cross[2];

					item->pre_w_cross[0] = pre_w_cross[0];
					item->pre_w_cross[1] = pre_w_cross[1];
					item->pre_w_cross[2] = pre_w_cross[2];
					item->fol_w_cross[0] = fol_w_cross[0];
					item->fol_w_cross[1] = fol_w_cross[1];
					item->fol_w_cross[2] = fol_w_cross[2];

					strcpy(item->pre_1_SubPOS, POSMap[pre_1_subpos]);
					strcpy(item->pre_2_SubPOS, POSMap[pre_2_subpos]);
					strcpy(item->pre_3_SubPOS, POSMap[pre_3_subpos]);
					strcpy(item->cur_SubPOS, POSMap[cur_subpos]);
					strcpy(item->fol_1_SubPOS, POSMap[fol_1_subpos]);
					strcpy(item->fol_2_SubPOS, POSMap[fol_2_subpos]);
					strcpy(item->fol_3_SubPOS, POSMap[fol_3_subpos]);

					strcpy(item->pre_1_POS, POSMap[pre_1_pos]);
					strcpy(item->pre_2_POS, POSMap[pre_2_pos]);
					strcpy(item->pre_3_POS, POSMap[pre_3_pos]);
					strcpy(item->cur_POS, POSMap[cur_pos]);
					strcpy(item->fol_1_POS, POSMap[fol_1_pos]);
					strcpy(item->fol_2_POS, POSMap[fol_2_pos]);
					strcpy(item->fol_3_POS, POSMap[fol_3_pos]);



					item->pre_3_SWL = pre_3_SWL;
					item->pre_2_SWL = pre_2_SWL;
					item->pre_1_SWL = pre_1_SWL;
					item->cur_SWL = cur_SWL;
					item->fol_1_SWL = fol_1_SWL;
					item->fol_2_SWL = fol_2_SWL;
					item->fol_3_SWL = fol_3_SWL;


					item->pre_3_WL = pre_3_WL;
					item->pre_2_WL = pre_2_WL;
					item->pre_1_WL = pre_1_WL;
					item->cur_WL = cur_WL;
					item->fol_1_WL = fol_1_WL;
					item->fol_2_WL = fol_2_WL;
					item->fol_3_WL = fol_3_WL;

//					item->F_syl_in_phrase[0] = syl_in_Ph_F[0];
//					item->F_syl_in_phrase[1] = syl_in_Ph_F[1];
//					item->B_syl_in_phrase[0] = syl_in_Ph_B[0];
//					item->B_syl_in_phrase[1] = syl_in_Ph_B[1];

//					strcpy(item->pre_1_Ph, PhTypeMap[pre_1_phtype]);
//					strcpy(item->fol_1_Ph, PhTypeMap[fol_1_phtype]);
//					item->fol_1_PhL = fol_1_phl;
//					item->pre_1_PhL = pre_1_phl;

					item->F_syl_in_sent[0] = F_syl_in_sent[0];
					item->F_syl_in_sent[1] = F_syl_in_sent[1];
					item->B_syl_in_sent[0] = B_syl_in_sent[0];
					item->B_syl_in_sent[1] = B_syl_in_sent[1];
					item->sent_length_in_syl = sent_len;
					item->pre_sent_length_in_syl = pre_sent_len;
					item->fol_sent_length_in_syl = fol_sent_len;








					if( pre_sw_cross[0]==1 )
					{
						item->pre_3_SWL = 0;
						item->pre_2_SWL = 0;
						item->pre_1_SWL = 0;
						strcpy(item->pre_1_SubPOS, POSMap[0]);
						strcpy(item->pre_2_SubPOS, POSMap[0]);
						strcpy(item->pre_3_SubPOS, POSMap[0]);
					}
					if( pre_sw_cross[1]==1 )
					{
						item->pre_3_SWL = 0;
						item->pre_2_SWL = 0;
						strcpy(item->pre_2_SubPOS, POSMap[0]);
						strcpy(item->pre_3_SubPOS, POSMap[0]);
					}
					if( pre_sw_cross[2]==1 )
					{
						item->pre_3_SWL = 0;
						strcpy(item->pre_3_SubPOS, POSMap[0]);
					}
					if( fol_sw_cross[0]==1 )
					{
						item->fol_3_SWL = 0;
						item->fol_2_SWL = 0;
						item->fol_1_SWL = 0;
						strcpy(item->fol_1_SubPOS, POSMap[0]);
						strcpy(item->fol_2_SubPOS, POSMap[0]);
						strcpy(item->fol_3_SubPOS, POSMap[0]);
					}
					if( fol_sw_cross[1]==1 )
					{
						item->fol_3_SWL = 0;
						item->fol_2_SWL = 0;
						strcpy(item->fol_2_SubPOS, POSMap[0]);
						strcpy(item->fol_3_SubPOS, POSMap[0]);
					}
					if( fol_sw_cross[2]==1 )
					{
						item->fol_3_SWL = 0;
						strcpy(item->fol_3_SubPOS, POSMap[0]);
					}




					if( pre_w_cross[0]==1 )
					{
						item->pre_3_WL = 0;
						item->pre_2_WL = 0;
						item->pre_1_WL = 0;
						strcpy(item->pre_1_POS, POSMap[0]);
						strcpy(item->pre_2_POS, POSMap[0]);
						strcpy(item->pre_3_POS, POSMap[0]);
					}
					if( pre_w_cross[1]==1 )
					{
						item->pre_3_WL = 0;
						item->pre_2_WL = 0;
						strcpy(item->pre_2_POS, POSMap[0]);
						strcpy(item->pre_3_POS, POSMap[0]);
					}
					if( pre_w_cross[2]==1 )
					{
						item->pre_3_WL = 0;
						strcpy(item->pre_3_POS, POSMap[0]);
					}
					if( fol_w_cross[0]==1 )
					{
						item->fol_3_WL = 0;
						item->fol_2_WL = 0;
						item->fol_1_WL = 0;
						strcpy(item->fol_1_POS, POSMap[0]);
						strcpy(item->fol_2_POS, POSMap[0]);
						strcpy(item->fol_3_POS, POSMap[0]);
					}
					if( fol_w_cross[1]==1 )
					{
						item->fol_3_WL = 0;
						item->fol_2_WL = 0;
						strcpy(item->fol_2_POS, POSMap[0]);
						strcpy(item->fol_3_POS, POSMap[0]);
					}
					if( fol_w_cross[2]==1 )
					{
						item->fol_3_WL = 0;
						strcpy(item->fol_3_POS, POSMap[0]);
					}

//					if( fol_ph_cross==1 )
//					{
					//	strcpy(item->fol_1_Ph, "x");
					//	item->fol_1_PhL = 0;
//					}
					HTS_lab_Add_item(p, item);
				}
			}


			// previous initial/final type






		}
	}


	pre_item = p->items_head;
	// insert sil
	item = HTS_lab_item_new();
	strcpy(item->cur_inifin, "sil");
	strcpy(item->pre_inifin, "sent_start");
	strcpy(item->fol_inifin, p->items_head->cur_inifin);
	item->lab_start = 0;
	item->lab_end = pre_item->lab_start;
	item->next = pre_item;
	pre_item->previous = item;
	p->items_head = item;
	item = pre_item;
	while(item->next != NULL)
	{
		strcpy(item->fol_inifin, item->next->cur_inifin);
		strcpy(item->pre_inifin, item->previous->cur_inifin);
		item = item->next;
	}
	strcpy(item->pre_inifin, item->previous->cur_inifin);
	strcpy(item->fol_inifin, "sil");
	// insert sil
	pre_item = HTS_lab_item_new();
	strcpy(pre_item->cur_inifin, "sil");
	strcpy(pre_item->pre_inifin, item->cur_inifin);
	strcpy(pre_item->fol_inifin, "sent_end");
	item->next = pre_item;
	pre_item->lab_start = item->lab_end;
	pre_item->lab_end = lab->items[lab->size-1].end_t;
	return 1;
}

/*




	bool MakeHTSlab(transx &trans, HTK_lab &lab, double pau_thr);
	int Find_lab_acording_to_transx(transx &trans, int ith_syl, HTK_lab &lab, int &start_in_lab, int &end_in_lab);
	bool PrintHTSlabfull(char *filename);
	bool PrintHTSlabfull_without_seg(char *filename);
	bool PrintHTSlabmono(char *filename);
	*/
void FormHTSlabelstring(ProsodyDataSet *p_data)
{

	HTS_lab_item *item;
	char tmpstr[32][1024];
	int i;
	item = p_data->hts_label->items_head;

	while( item != NULL )
	{
//----------cchang-----------------//


		sprintf(tmpstr[0],"%s-%s+%s^%d",item->pre_inifin,item->cur_inifin,item->fol_inifin,item->inifin_in_syl);
		sprintf(tmpstr[1],"=%d@%d#%d",item->pre_tone,item->cur_tone,item->fol_tone);
		sprintf(tmpstr[2],"&%d_%d|%d_%d/p:%d_%d/q:%d_%d", item->F_syl_in_subword[0], item->F_syl_in_subword[1], item->B_syl_in_subword[0], item->B_syl_in_subword[1],item->F_syl_in_word[0],item->F_syl_in_word[1],item->B_syl_in_word[0],item->B_syl_in_word[1]);
		//v2 fprintf(fp,"&%d_%d|%d_%d", item->F_syl_in_subword[0], item->F_syl_in_subword[1], item->B_syl_in_subword[0], item->B_syl_in_subword[1]);
		sprintf(tmpstr[3],"/a:%s_%d/b:%s_%d/c:%s_%d/d:%s_%d/e:%s_%d/f:%s_%d/g:%s_%d",item->pre_3_SubPOS,item->pre_3_SWL,item->pre_2_SubPOS,item->pre_2_SWL,item->pre_1_SubPOS,item->pre_1_SWL,item->cur_SubPOS,item->cur_SWL,item->fol_1_SubPOS,item->fol_1_SWL,item->fol_2_SubPOS, item->fol_2_SWL, item->fol_3_SubPOS, item->fol_3_SWL);
		sprintf(tmpstr[4],"/A:%s_%d/B:%s_%d/C:%s_%d/D:%s_%d/E:%s_%d/F:%s_%d/G:%s_%d",item->pre_3_POS,item->pre_3_WL, item->pre_2_POS,item->pre_2_WL,item->pre_1_POS,item->pre_1_WL,item->cur_POS,item->cur_WL,item->fol_1_POS,item->fol_1_WL,item->fol_2_POS,item->fol_2_WL,item->fol_3_POS,item->fol_3_WL);
		sprintf(tmpstr[5],"/H:%d_%d/I:%d_%d",item->F_syl_in_phrase[0],item->F_syl_in_phrase[1],item->B_syl_in_phrase[0],item->B_syl_in_phrase[1]);
		sprintf(tmpstr[6],"/J:%s_%d/K:%s_%d",item->pre_1_Ph,item->pre_1_PhL,item->fol_1_Ph,item->fol_1_PhL);

		if(item->pre_PM_id==0&&item->fol_PM_id==0)
			sprintf(tmpstr[7],"/L:%s/M:%s",item->pre_PM,item->fol_PM);
		if(item->pre_PM_id!=0&&item->fol_PM_id==0)
			sprintf(tmpstr[7],"/L:%d/M:%s",item->pre_PM_id,item->fol_PM);
		if(item->pre_PM_id==0&&item->fol_PM_id!=0)
			sprintf(tmpstr[7],"/L:%s/M:%d",item->pre_PM,item->fol_PM_id);
		if(item->pre_PM_id!=0&&item->fol_PM_id!=0)
			sprintf(tmpstr[7],"/L:%d/M:%d",item->pre_PM_id,item->fol_PM_id);

		sprintf(tmpstr[8],"/N:%d/O:%d/P:%d/Q:%d",item->F_syl_in_sent[0],item->B_syl_in_sent[0],item->sent_length_in_syl,item->fol_sent_length_in_syl);
		// v2 fprintf(fp,"/N:%d/O:%d\n",item->F_syl_in_sent[0],item->B_syl_in_sent[0]);
		memset(item->label_string, 0, sizeof(char)*1024);

		for(i=0;i<9;i++) {
			strcat(item->label_string, tmpstr[i]);
		}

//----------cchang------------------//
		item = item->next;
	}
}

void GenHTSLabel(ProsodyDataSet *p_data, double pauth)
{
	p_data->hts_label = (HTS_lab *) calloc(1, sizeof(HTS_lab));
	MakeHTSlab(p_data->hts_label, p_data->transx, p_data->htklab, pauth);
	FormHTSlabelstring(p_data);
	p_data->pauth = pauth;
}


bool PrintHTSlabfull_without_seg(ProsodyDataSet *p_data, char *filename)
{
	FILE *fp = fopen(filename, "w");
	HTS_lab_item *item;
	if( fp == NULL )
	{
		printf("Cannot open %s\n", filename);
		return false;
	}
	item = p_data->hts_label->items_head;

	while( item != NULL )
	{

		fprintf(fp, "%s\n", item->label_string);

		item = item->next;
	}

	fclose(fp);

	return true;
}


bool SaveXLabel(ProsodyDataSet *p_data, char *fn)
{
	long HTK_unit = 10000000;
	long cur_t = 0, t = 0;
	int i, j;
	HTS_lab_item *p;
	FILE *fp_xlab = fopen(fn, "w");
	if( !fp_xlab ) {
		fprintf(stderr, "Cannot save %s\n", fn);
		return false;
	}
	// sent. start
	fprintf(fp_xlab, "ver1.0\n");
	p = p_data->hts_label->items_head;
	fprintf(fp_xlab,"sil 1 %s %f 0 0 0 0 0\n", p->label_string, 1.0);
	cur_t = cur_t + HTK_unit*1 + 1;
	p = p->next;

	for(i=0;i<(p_data->DataNum);i++)
	{
		///////// syl part

		t = (int)( (p_data->prosodys[i].duration) * HTK_unit);
		fprintf(fp_xlab,"%s 2 %s ",p_data->lins[i].syl, p->label_string);
		p = p->next;
		fprintf(fp_xlab,"%s ",p->label_string);
		fprintf(fp_xlab,"%f %f %f %f %f %f\n",p_data->prosodys[i].duration, p_data->prosodys[i].lf0_OE[0], p_data->prosodys[i].lf0_OE[1], p_data->prosodys[i].lf0_OE[2], p_data->prosodys[i].lf0_OE[3], p_data->prosodys[i].energy_level);
		p = p->next;
		cur_t = cur_t + t + 1;


		///////// pause part
		if( (p_data->prosodys[i].pause) > p_data->pauth )
		{
			t = (int)( (p_data->prosodys[i].pause) * HTK_unit);
			fprintf(fp_xlab,"sil 1 %s %f 0 0 0 0 0\n", p->label_string, p_data->prosodys[i].pause);
			p = p->next;
			cur_t = cur_t + t + 1;
		}

	}
	// sent. end
	fprintf(fp_xlab,"sil 1 %s %f 0 0 0 0 0\n", p->label_string, 1.0);



	fclose(fp_xlab);
}


void SaveProsodyFeaFp(ProsodyDataSet *p_data, FILE *fp)
{
	int i;
	int j = 0;
	for(i=0;i<p_data->transx->size;i++) {
		if( !(((int)(p_data->transx->items[i].code/1000))==9) ) {
			fprintf(fp, "%s\t%s\t", p_data->transx->filename, p_data->transx->items[i].syl);
			fprintf(fp, "%.15f\t%.15f\t%.15f\t%.15f\t", p_data->prosodys[j].lf0_OE[0], p_data->prosodys[j].lf0_OE[1], p_data->prosodys[j].lf0_OE[2], p_data->prosodys[j].lf0_OE[3]);
			fprintf(fp, "%.15f\t%.15f\t%.15f\t", p_data->prosodys[j].duration, p_data->prosodys[j].energy_level, p_data->prosodys[j].pause);
			fprintf(fp, "%d\t%d\t%d\t%d\n", p_data->prosodys[j].PS_p, p_data->prosodys[j].PS_q, p_data->prosodys[j].PS_r, p_data->prosodys[j].B);
			fflush(fp);
			j ++;
		}
	}
}

#endif