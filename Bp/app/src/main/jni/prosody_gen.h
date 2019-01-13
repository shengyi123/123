#include"data_format.h"



int Trans2LinData(ProsodyDataSet *p_data, char *transName);

void GetLeafNodeTag_forB(ProsodyDataSet *p_data);

void BreakPrediction(ProsodyDataSet *p_data, float SR);

void GetLeafNodeTag_forPS(ProsodyDataSet *p_data);

void ProsodicStatePrediction_p(ProsodyDataSet *p_data, float SR);

void ProsodicStatePrediction_q(ProsodyDataSet *p_data, float SR);

void ProsodicStatePrediction_r(ProsodyDataSet *p_data, float SR);

void PausePrediction(ProsodyDataSet *p_data, float SR);

void LogF0ContourPrediction(ProsodyDataSet *p_data, float SR);

void DurationPrediction(ProsodyDataSet *p_data, float SR);

void EnergyLevelPrediction(ProsodyDataSet *p_data, float SR);

void GenHTKLabel(ProsodyDataSet *p_data);

int  SaveHTKLabelFn(ProsodyDataSet *p_data, char *fn);

void GenHTSLabel(ProsodyDataSet *p_data, double pauth);

void FormHTSlabelstring(ProsodyDataSet *p_data);

jboolean PrintHTSlabfull_without_seg(ProsodyDataSet *p_data, char *filename);


jboolean SaveXLabel(ProsodyDataSet *p_data, char *fn);

void FormHTSlabelstring(ProsodyDataSet *p_data);

void SaveProsodyFeaFp(ProsodyDataSet *p_data, FILE *fp);