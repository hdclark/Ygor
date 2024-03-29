//YgorEnvironment.cc - Routines for collecting info about the local (system) environment. 

#include "YgorDefinitions.h"

#ifdef YGOR_USE_LINUX_SYS

#include <cstdio>      //Needed for getchar();
#include <iostream>
#include <string>
#include <utility>     //Needed for std::pair.

#include <unistd.h>

#ifdef __linux__
    #include <linux/fb.h>  //Needed for Linux framebuffer ioctl and struct definitions.
#else
    #error "Need to specify where 'fb.h' is for this non-Linux environment"
#endif


#include <fcntl.h>     //Needed for open(3).
#include <sys/ioctl.h> //Needed for ioctl() for determining terminal char, framebuffer dims.
#include <sys/wait.h>  //Needed for system_bash().
#include <cerrno>     //Needed for system_bash().
#include <csignal>    //Needed for system_bash().
#include <cstdint>

#include "YgorEnvironment.h"
#include "YgorMisc.h"    //Needed for Execute_Command_In_Pipe.
#include "YgorLog.h"
#include "YgorString.h"  //Needed for Xtostring.



//--------------------------------------------------------------------------------------------------------
//----------------------------------------- /bin/bash system(3) ------------------------------------------
//--------------------------------------------------------------------------------------------------------
int system_bash(const char *command){
    //This was originally copied from http://man7.org/tlpi/code/online/dist/procexec/system.c
    // and was written by Michael Kerrisk in 2010. I have used the online version as a scaffolding to 
    // create my own version.
    sigset_t blockMask, origMask;
    struct sigaction saIgnore, saOrigQuit, saOrigInt, saDefault;
    pid_t childPid;
    int status, savedErrno;

    if (command == nullptr)                /* Is a shell available? */
        return system_bash(":") == 0;

    /* The parent process (the caller of system_bash()) blocks SIGCHLD
       and ignore SIGINT and SIGQUIT while the child is executing.
       We must change the signal settings prior to forking, to avoid
       possible race conditions. This means that we must undo the
       effects of the following in the child after fork(). */

    sigemptyset(&blockMask);            /* Block SIGCHLD */
    sigaddset(&blockMask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blockMask, &origMask);

    saIgnore.sa_handler = SIG_IGN;      /* Ignore SIGINT and SIGQUIT */
    saIgnore.sa_flags = 0;
    sigemptyset(&saIgnore.sa_mask);
    sigaction(SIGINT, &saIgnore, &saOrigInt);
    sigaction(SIGQUIT, &saIgnore, &saOrigQuit);

    switch (childPid = ::fork()) {
    case -1: /* fork() failed */
        status = -1;
        break;          /* Carry on to reset signal attributes */

    case 0: /* Child: exec command */

        /* We ignore possible error returns because the only specified error
           is for a failed exec(), and because errors in these calls can't
           affect the caller of system_bash() (which is a separate process) */

        saDefault.sa_handler = SIG_DFL;
        saDefault.sa_flags = 0;
        sigemptyset(&saDefault.sa_mask);

        if (saOrigInt.sa_handler != SIG_IGN)
            sigaction(SIGINT, &saDefault, nullptr);
        if (saOrigQuit.sa_handler != SIG_IGN)
            sigaction(SIGQUIT, &saDefault, nullptr);

        sigprocmask(SIG_SETMASK, &origMask, nullptr);

        ::execl("/bin/bash", "bash", "-c", command, (char *) nullptr);
        ::_exit(127);                     /* We could not exec the shell */

    default: /* Parent: wait for our child to terminate */

        /* We must use waitpid() for this task; using wait() could inadvertently
           collect the status of one of the caller's other children */

        while (::waitpid(childPid, &status, 0) == -1) {
            if (errno != EINTR) {       /* Error other than EINTR */
                status = -1;
                break;                  /* So exit loop */
            }
        }
        break;
    }

    /* Unblock SIGCHLD, restore dispositions of SIGINT and SIGQUIT */

    savedErrno = errno;                 /* The following may change 'errno' */

    sigprocmask(SIG_SETMASK, &origMask, nullptr);
    sigaction(SIGINT, &saOrigInt, nullptr);
    sigaction(SIGQUIT, &saOrigQuit, nullptr);

    errno = savedErrno;

    return status;
}

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------- Simple Input ---------------------------------------------
//--------------------------------------------------------------------------------------------------------
void Wait_For_Enter_Press(void){
    int c;
    std::cout << "Please press [enter] to continue" << std::endl;
    fflush( stdout );
    do{
        c = getchar();
        if((c != '\n') && (c != EOF)) std::cout << "Waiting for user to press [enter] to continue" << std::endl;
        fflush( stdout );
    }while((c != '\n') && (c != EOF));
    return;
}


//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Framebuffer ---------------------------------------------
//--------------------------------------------------------------------------------------------------------
std::pair<int64_t, int64_t> Get_Framebuffer_Pixel_Dimensions(int fbN, bool Virtual /* = false*/){
    //NOTE: 'Virtual' refers to the entire buffer, which may include both visible buffers in
    // a double-buffer arrangement. The default is the visible buffer, which corresponds to
    // what is visible on the screen.
    //
    //   ||============================================= virtual ==================|| }
    //   ||              }                                                         || }
    //   ||              } yoffset                                                 || }
    //   ||              }                                                         || }
    //   ||          |------------------- visible --------|  }                     || }
    //   ||          |                                    |  }                     || }
    //   ||          |                                    |  }                     || } yres_virtual
    //   ||__________|                                    |  } yres                || }
    //   || xoffset  |                                    |  }                     || }
    //   ||          |                                    |  }                     || }
    //   ||          |------------------------------------|  }                     || }
    //   ||          ______________________________________                        || }
    //   ||                         xres                                           || }
    //   ||                                                                        || }
    //   ||========================================================================|| }
    //    __________________________________________________________________________
    //                                         xres_virtual
    //
/*
THIS IS the original way I was doing it. It assumes we only care about /dev/fb0 and uses regex.
Might be OK to fall back on the virtual_size file, because it is super convenient.

    std::string dimens = Execute_Command_In_Pipe("cat /sys/class/graphics/fb0/virtual_size");
    if(dimens.size() == 0){
        dimens = Execute_Command_In_Pipe("cat /sys/devices/virtual/graphics/fbcon/subsystem/fb0/virtual_size");
    }
    if(dimens.size() == 0){
        YLOGWARN("Unable to determine the dimensions of the framebuffer");
        return std::pair<int64_t, int64_t>(-1,-1);
    }
    const std::string Ws = GetFirstRegex( dimens, R"***(^([0-9]{3,})[,])***" );
    const std::string Hs = GetFirstRegex( dimens, R"***([,]([0-9]{3,}))***"  );
    const int64_t W = Is_String_An_X<int64_t>(Ws) ? stringtoX<int64_t>(Ws) : -1;
    const int64_t H = Is_String_An_X<int64_t>(Hs) ? stringtoX<int64_t>(Hs) : -1;
    return std::pair<int64_t, int64_t>( W, H );
*/
    std::pair<int64_t, int64_t> out(-1, -1);

#ifdef __linux__
    //This is specific to Linux. It requires /usr/include/linux/fb.h struct/macro defs.
    // If needed on another system, consider porting. In particular, Mingw might have a
    // suitable replacement... Not sure about the BSDs.
    int fb_fd;

    //Construct the appropriate framebuffer filename. Try open it.
    const std::string Filename_FB("/dev/fb"_s + Xtostring(fbN));
    if((fb_fd = open(Filename_FB.c_str(), O_RDONLY)) < 0) return out;

    //Populate a struct using ioctl. See /usr/include/linux/fb.h for constants.
    // We only care about the 'variable' framebuffer info, not the fixed info.
    struct fb_var_screeninfo vs;
    if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &vs) != -1){
        //Note: we use the visible framebuffer dimensions, in contrast to the virtual.
        // The difference is that the visible will fit in the screen
        const int64_t W = static_cast<int64_t>(Virtual ? vs.xres_virtual : vs.xres);
        const int64_t H = static_cast<int64_t>(Virtual ? vs.yres_virtual : vs.yres);
        out = std::pair<int64_t, int64_t>(W,H);
    }else{
        YLOGWARN("Framebuffer file appears to exist, but ioctl on it failed");
    }
    close(fb_fd);
#else
    YLOGWARN("This routine is not presently available on non-Linux systems. Returning negative dimensions"); 
#endif

    return out;
}


//--------------------------------------------------------------------------------------------------------
//------------------------------------------- Terminal Dimensions ----------------------------------------
//--------------------------------------------------------------------------------------------------------
std::pair<int64_t, int64_t> Get_Terminal_Char_Dimensions(void){
    //Note: *Always* handle the case of getting -1's. It is VALID for this function to always return them!
    //Note: If porting this function and having problems, it is valid to always just return -1's!
    //
    //See http://stackoverflow.com/questions/1022957/getting-terminal-width-in-c for the basis of this
    // function, and a few alternatives if this doesn't work.
    struct winsize w;
    int64_t W(-1), H(-1);
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1){
        W = static_cast<int64_t>(w.ws_col);
        H = static_cast<int64_t>(w.ws_row);
    }
    return std::pair<int64_t, int64_t>(W,H); //Width and Height in terms of # of chars.
}

//--------------------------------------------------------------------------------------------------------
//----------------------------------------------- X-related ----------------------------------------------
//--------------------------------------------------------------------------------------------------------
bool Is_X_Running_And_In_Focus(void){
    return ( (Execute_Command_In_Pipe("echo -n \"$DISPLAY\"")).size() != 0 );
}


//--------------------------------------------------------------------------------------------------------
//-------------------------------------------- Memory-related --------------------------------------------
//--------------------------------------------------------------------------------------------------------
double Amount_Of_Total_Memory_MB(void){
    //NOTE: This should return the maximum RAM size, in MB.
    //
    //NOTE: This does NOT consider swap space.

    //For Linux.
    #if defined _SC_PHYS_PAGES && defined _SC_PAGESIZE
        //std::cout << "The number of pages in total is: " << sysconf(_SC_PHYS_PAGES) << std::endl;
        //std::cout << "The size of each page is:         " << sysconf(_SC_PAGESIZE) << std::endl;
        double tot = static_cast<double>(sysconf(_SC_PHYS_PAGES))/(1024.0*1024.0);
        tot *= static_cast<double>(sysconf(_SC_PAGESIZE));
        //std::cout << "    (which means total: " << tot << " MB" << std::endl;
        if(tot >= 0.0) return tot;
    #endif

    //For BSDs (including darwin).
    #if HAVE_SYSCTL && defined HW_USERMEM
        unsigned int physmem;
        size_t len = sizeof physmem;
        static int mib[2] = { CTL_HW, HW_PHYSMEM };

        if( (sysctl(mib, ARRAY_SIZE(mib), &physmem, &len, NULL, 0) == 0) && (len == sizeof(physmem))){
            return static_cast<double>(physmem)/(1024.0*1024.0);
        }
    #endif

    //Information unavailable.
    return -1.0;
}


double Amount_Of_Totally_Free_Memory_MB(void){
    //NOTE: This is NOT the total amount of memory one could allocate (heap) before exhausting memory.
    // Rather - this is the total amount of memory a process could allocate (heap) without forcing the
    // kernel to free cached memory chunks. This is a "conservative, easy estimate" of available heap.
    //
    //NOTE: This does NOT consider swap space.
    //
    //NOTE: Inspired by http://www.opensource.apple.com/source/gcc_os/gcc_os-1671/libiberty/physmem.c

    //For Linux.
    #if defined _SC_AVPHYS_PAGES && defined _SC_PAGESIZE
        //std::cout << "The number of pages available is: " << sysconf(_SC_AVPHYS_PAGES) << std::endl;
        //std::cout << "The size of each page is:         " << sysconf(_SC_PAGESIZE) << std::endl;
        double tot = static_cast<double>(sysconf(_SC_AVPHYS_PAGES))/(1024.0*1024.0);
        tot *= static_cast<double>(sysconf(_SC_PAGESIZE));
        //std::cout << "    (which means total available: " << tot << " MB" << std::endl;
        if(tot >= 0.0) return tot;
    #endif

    //For BSDs (including darwin).
    #if HAVE_SYSCTL && defined HW_USERMEM
        unsigned int usermem;
        size_t len = sizeof usermem;
        static int mib[2] = { CTL_HW, HW_USERMEM };

        if( (sysctl(mib, ARRAY_SIZE(mib), &usermem, &len, NULL, 0) == 0) && (len == sizeof(usermem))){
            return static_cast<double>(usermem)/(1024.0*1024.0);
        }
    #endif

    //Information unavailable. Check for system-specific work around.
    return -1.0;
}

#endif //YGOR_USE_LINUX_SYS

