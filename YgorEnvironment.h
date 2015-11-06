//YgorEnvironment.h

#ifndef YGOR_ENVIRONMENT_HDR_GRD_H
#define YGOR_ENVIRONMENT_HDR_GRD_H

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
//----------------------------------------- Simple Email Sending -----------------------------------------
//--------------------------------------------------------------------------------------------------------
bool Send_Simple_SMTP_Email(const std::string &TO, const std::string &subject, const std::string &msg);          //DEPRECATED and BROKEN! Do not use!

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




//-------- Other (? Should this be here???)

//----------------------------------- Print color text in a console using a simple 'setter' side-effect call. --------------------------
/*#include <stdio.h>

#define RESET           0
#define BRIGHT          1
#define DIM             2
#define UNDERLINE       3
#define BLINK           4
#define REVERSE         7
#define HIDDEN          8

#define BLACK           0
#define RED             1
#define GREEN           2
#define YELLOW          3
#define BLUE            4
#define MAGENTA         5
#define CYAN            6
#define WHITE           7

void textcolor(int attr, int fg, int bg);
int main()
{       textcolor(BRIGHT, RED, WHITE);
        printf("In color\n");
        textcolor(RESET, WHITE, BLACK);
        return 0;
}

void textcolor(int attr, int fg, int bg)
{       char command[13];

        //Command is the control command to the terminal.
        sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
        printf("%s", command);
}
*/

#endif
