#include <stdio.h>
#include <stdlib.h>
#define BTypeNum 7
#define PSNum 16


typedef struct _TRANS
{
	char syl[32];
	int  code;
	int  index;
	int  pos;
	int	 subindex;
	int  subpos;
	int  syl_idx;
	int is_PM;
	int PM_id;

}TRANS;

typedef struct _TRANSStream
{
	int size;
	TRANS *items;
	char filename[1024];
	int transx_type;
} TRANSStream;

typedef struct _LinDataStream
{
	char syl[16];
	char initial[16];
	char final[16];
	int  code411;
	int  index;
	int  pos;
	int	 subindex;
	int  subpos;
	int  tone;
	int  PM;
	char  PMchar[16];

}LinDataStream;

typedef struct _ProsodyStream
{
	float lf0_OE[4];
	float pause;
	float duration;
	float energy_level;
	int B;
	int PS_p;
	int PS_q;
	int PS_r;

}ProsodyStream;

typedef struct _HTK_label_item
{
	long begin_t;
	long end_t;
	char inifin[32];
	char syllable[32];
} HTK_label_item;

typedef struct _HTKLabStream
{
	int size;
	HTK_label_item *items;
} HTKLabStream;



typedef struct _HTS_lab_item
{
	char label_string[1024];
	char CH[16];
	int code;
	int index;
	int pos;
	int subindex;
	int subpos;



	struct _HTS_lab_item *next;
	struct _HTS_lab_item *previous;

	// lab
	int lab_start;
	int lab_end;


	// phone level
	// initial-final level
	char pre_inifin[64];// previous initial/final type
	char cur_inifin[64];// current initial/final type
	char fol_inifin[64];// following initial/final type
	int inifin_in_syl;// initial/final position in a syllable
	// syllable level
	int pre_tone;// Lexical tone of previous syllable
	int cur_tone;// Lexical tone of current syllable
	int fol_tone;// Lexical tone of following syllable
	int F_syl_in_subword[2];// Syllable position in a lexical subword (forward)
	int B_syl_in_subword[2];// Syllable position in a lexical subword (backward)
	int F_syl_in_word[2];// Syllable position in a lexical word (forward)
	int B_syl_in_word[2];// Syllable position in a lexical word (backward)
	// word level
	char pre_PM[64];// PM type preceding current syllable
	char fol_PM[64];// PM type following current syllable
	int pre_PM_id;// PM id of peceding of current syllable
	int fol_PM_id;// PM id of following of current syllable


	char pre_3_SubPOS[64];// 47-type POS (Academia Sinica) of word preceding-preceding the previous subword
	char pre_2_SubPOS[64];// 47-type POS (Academia Sinica) of word preceding the previous subword
	char pre_1_SubPOS[64];// 47-type POS of previous subword
	char cur_SubPOS[64];// 47-type POS of current subword
	char fol_1_SubPOS[64];// 47-type POS of following subword
	char fol_2_SubPOS[64];// 47-type POS of the next word following the following subword
	char fol_3_SubPOS[64];// 47-type POS of the next-next word following the following subword

	char pre_3_POS[64];// 47-type POS (Academia Sinica) of word preceding-preceding the previous word
	char pre_2_POS[64];// 47-type POS (Academia Sinica) of word preceding the previous word
	char pre_1_POS[64];// 47-type POS of previous word
	char cur_POS[64];// 47-type POS of current word
	char fol_1_POS[64];// 47-type POS of following word
	char fol_2_POS[64];// 47-type POS of the next word following the following word
	char fol_3_POS[64];// 47-type POS of the next-next word following the following word

	int pre_3_SWL;// Word length of subword preceding-preceding the previous subword
	int pre_2_SWL;// Word length of subword preceding the previous subword
	int pre_1_SWL;// Word length of previous subword
	int cur_SWL;// Word length of current subword
	int fol_1_SWL;// Word length of following subword
	int fol_2_SWL;// Word length of the next subword following the following subword
	int fol_3_SWL;// Word length of the next-next subword following the following subword
	int pre_sw_cross[3];// is the previous subword crossing a sentence
	int fol_sw_cross[3];// is the following subword crossing a sentence

	int pre_3_WL;// Word length of word preceding-preceding the previous word
	int pre_2_WL;// Word length of word preceding the previous word
	int pre_1_WL;// Word length of previous word
	int cur_WL;// Word length of current word
	int fol_1_WL;// Word length of following word
	int fol_2_WL;// Word length of the next word following the following word
	int fol_3_WL;// Word length of the next-next word following the following word
	int pre_w_cross[3];// is the previous word crossing a sentence
	int fol_w_cross[3];// is the following word crossing a sentence
	// phrase level
	char pre_1_Ph[64];// phrase type of previous phrase;
	char fol_1_Ph[64];// phrase type of following phrase
	int pre_1_PhL;// phrase length of previous phrase;
	int fol_1_PhL;// phrase length of following phrase
	int F_syl_in_phrase[2];// Syllable position in a phrase (forward)
	int B_syl_in_phrase[2];// Syllable position in a phrase (backward)
	// sentence level
	int F_syl_in_sent[2];// Syllable position in a sentence (forward)
	int B_syl_in_sent[2];// Syllable position in a sentence (backward)
	int sent_length_in_syl;// Sentence length in syllable
	int pre_sent_length_in_syl;// Previous sentence length in syllable
	int fol_sent_length_in_syl;// Following sentence length in syllable
} HTS_lab_item;



typedef struct _HTS_lab
{
	int num_of_HTS_lab_item;
	HTS_lab_item *items_head;	
} HTS_lab;



typedef struct _ProsodyDataSet
{
	TRANSStream *transx;
	LinDataStream *lins;
	ProsodyStream *prosodys;
	HTKLabStream *htklab;
	HTS_lab *hts_label;
	int  *LeafNodeTag_forB;
	int  *LeafNodeTag_for_p;
	int  *LeafNodeTag_for_q;
	int  *LeafNodeTag_for_r;
	int  DataNum;
	float pauth;
}ProsodyDataSet;






