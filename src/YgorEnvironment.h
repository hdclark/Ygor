//YgorEnvironment.h

#ifndef YGOR_ENVIRONMENT_HDR_GRD_H
#define YGOR_ENVIRONMENT_HDR_GRD_H

#include "YgorDefinitions.h"

#ifdef YGOR_USE_LINUX_SYS

#include <string>
#include <utility>

//--------------------------------------------------------------------------------------------------------
//----------------------------------------- /bin/bash system(3) ------------------------------------------
//--------------------------------------------------------------------------------------------------------
int system_bash(const char *command);

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------- Simple Input ---------------------------------------------
//--------------------------------------------------------------------------------------------------------
void Wait_For_Enter_Press(void);

//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Framebuffer ---------------------------------------------
//--------------------------------------------------------------------------------------------------------
std::pair<long int, long int> Get_Framebuffer_Pixel_Dimensions(int fbN, bool Virtual = false);

//--------------------------------------------------------------------------------------------------------
//------------------------------------------- Terminal Dimensions ----------------------------------------
//--------------------------------------------------------------------------------------------------------
std::pair<long int, long int> Get_Terminal_Char_Dimensions(void); //W,H.

//--------------------------------------------------------------------------------------------------------
//----------------------------------------------- X-related ----------------------------------------------
//--------------------------------------------------------------------------------------------------------
bool Is_X_Running_And_In_Focus(void);

//--------------------------------------------------------------------------------------------------------
//-------------------------------------------- Memory-related --------------------------------------------
//--------------------------------------------------------------------------------------------------------
double Amount_Of_Total_Memory_MB(void);
double Amount_Of_Totally_Free_Memory_MB(void);


#endif //YGOR_USE_LINUX_SYS

#endif //YGOR_ENVIRONMENT_HDR_GRD_H

