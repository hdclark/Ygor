//Test_Noise.cc - A simple program for testing the 3D Perlin noise function.
//
//A 2D slice of the output is displayed via the terminal. There are no input
// facilities, so the process will need to be terminated with a [CTRL]+[C] or 
// similar.

#include <iostream>
#include <unistd.h>    //Needed for usleep(), STDOUT_FILENO.
//#include <sys/ioctl.h> //Needed for ioctl()
#include <cmath>
#include <signal.h>

#include "YgorNoise.h"
#include "YgorEnvironment.h"

float scale_time_with_time(float t){
    return sin(t);
}

//This function allows us to "zoom" a spacial coordinate with time.
float scale_with_time(float q, float t){
    t = scale_time_with_time(t*2);

    return q*( 1.0  +  3.0*exp(-t/0.5)  +    10.0*(1.0 - exp(-t/3.0)) );
    //      normal    zooms in initially.      zooms out later.
}

/*
int get_terminal_width(void){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

int get_terminal_height(void){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
}
*/

void SignalHandler(int sig){
    std::cout << "\033[m" << std::endl;   //Attempt to return the terminal attributes to normal and exit.
    exit(0);
    return;
}

int main(int argc, char **argv){

    //Intercept the kill signal to return the terminal's state to something sane.
    // This will ensure the terminal gets nicely reset when we kill the process.
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGHUP, SignalHandler);
    signal(SIGQUIT, SignalHandler);
    signal(SIGKILL, SignalHandler);
    signal(SIGPIPE, SignalHandler);
    signal(SIGCHLD, SignalHandler);

    const auto term_dims = Get_Terminal_Char_Dimensions();
    const long int cols = term_dims.first;
    const long int rows = term_dims.second;
    //std::cout << "There are " << cols << " columns." << std::endl;
    //std::cout << "There are " << rows << " rows." << std::endl;

    std::cout << "\033[" << "?25l";   //Hide the cursor.
    std::cout << "\033[" << "2J";     //Clear the (entire) screen.

    float vec[3];
    float f;
    int f_int;

    float x_ofst = 0.0, x_dir, y_dir, z_dir;
    float y_ofst = 0.0;

    for(float t = 0.0; t < 1E6; t+=1.0/30.0){

        const double theta = M_PI*(t - cos(t));

        for(long int i = 0; i < rows; ++i){
            for(long int j = 0; j < cols; ++j){
                const float xx = static_cast<float>(2*i - rows)/static_cast<float>(2*rows);  //A number in [0,rows)
                const float yy = static_cast<float>(2*j - cols)/static_cast<float>(2*cols);  //A number in [0,cols)

                const float x = cos(theta)*xx - sin(theta)*yy;
                const float y = sin(theta)*xx + cos(theta)*yy;

                vec[0] = scale_with_time(x,t) + 3.0*cos(t);
                vec[1] = scale_with_time(y,t) + 3.0*sin(t);
                vec[2] = t; //scale_time_with_time(t);
    
                f = Perlin_Noise_3D(vec); //f is within [-1,1]. 
                f = 0.5*(f+1.0); //f is within [0,1].

                //Dump out the float value.
                //std::cout << f << " ";

                //Quantize the output for background colors. -------
//                f_int = static_cast<int>( f * 8.99 ); //f_int is one of {0,1,...,8}.
//                f_int += 40; //f_int is one of {40,41,...,48}.       
  
                //Quantize the output for foreground colors. -------  0-255
//                f_int = static_cast<int>( f * 254.99 ); //f_int is one of {0,1,...,254}.

                //Quantize the output for foreground colors. -------  232-255  (Greyscale.)
//                f_int = static_cast<int>( f * 23.99 ); //f_int is one of {0,1,...,23}.
//                f_int += 232;      

                //Quantize the output for foreground colors. -------  16-21  (Bluescale.)
                f_int = static_cast<int>( f * 5.99 );
                f_int += 16;
 
 
                //Move the cursor to the appropriate position.
                std::cout << "\033[";
                std::cout << i;
                std::cout << ";";
                std::cout << j;
                std::cout << "H";

                //Dump the quantized background color.
//                std::cout << "\033[";
//                std::cout << f_int;
//                std::cout << "m \033[m";

                //Dump a colored character.
                std::cout << "\033[38;05;";
                std::cout << f_int;
                std::cout << "m";
                std::cout << "O\033[m";



            }
            //std::cout << std::endl;
        }
        //We want roughly 30 fps, so as a first approx attempt to sleep for 1/30th of a sec.
        usleep(1E6 * 1.0/30.0);
    }
    std::cout << "\033[" << "?25h";   //Show the cursor.

    return 0;
}








