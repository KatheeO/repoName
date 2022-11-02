#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "options.h"

//macros for normal, fast and slow copy modes
#define NORMAL 100
#define SLOW 1
#define FAST 100000



void FatalError(char c, const char* msg, int exit_status);
void PrintCopymasterOptions(struct CopymasterOptions* cpm_options);
void setPermissions(struct CopymasterOptions cpm_options);
void checkInode(struct CopymasterOptions cpm_options);
void makeLink(struct CopymasterOptions cpm_options);
void trun(struct CopymasterOptions cpm_options);
void makeSparse(struct CopymasterOptions cpm_options);
void printDirectory(struct CopymasterOptions cpm_options);
void fast_copy(struct CopymasterOptions cpm_options);
void slow_copy(struct CopymasterOptions cpm_options);

//copy funkcia obsahuje celé spracovanie funkcií prepínačov, teda je to hlavná časť programu
void copy(struct CopymasterOptions cpm_options);


int main(int argc, char* argv[])
{
    //initial parse of selected options
    struct CopymasterOptions cpm_options = ParseCopymasterOptions(argc, argv);

    //-------------------------------------------------------------------
    // Kontrola hodnot prepinacov
    //-------------------------------------------------------------------

    // Vypis hodnot prepinacov odstrante z finalnej verzie
    
    PrintCopymasterOptions(&cpm_options);

    //-------------------------------------------------------------------
    // Osetrenie prepinacov pred kopirovanim
    //-------------------------------------------------------------------
    
    //fast and slow collision
    if (cpm_options.fast && cpm_options.slow) {
        fprintf(stderr, "CHYBA PREPINACOV\n"); 
        //exit(EXIT_FAILURE);
        return 42;
    }
    //create, overwrite and append collision
    if ( (cpm_options.create && cpm_options.overwrite) || (cpm_options.create && cpm_options.append) || (cpm_options.overwrite && cpm_options.append)){
        fprintf(stderr, "CHYBA PREPINACOV\n"); 
       // exit(EXIT_FAILURE);
        return 42;
    }
    //lseek and append collision
    if ( (cpm_options.lseek && cpm_options.append)){
        fprintf(stderr, "CHYBA PREPINACOV\n");
        return 42;
    }
    //sparse options --- isn't combined with anything
    if (cpm_options.sparse && !cpm_options.fast && !cpm_options.slow && !cpm_options.create && !cpm_options.overwrite && !cpm_options.append && !cpm_options.lseek && !cpm_options.directory && !cpm_options.delete_opt && !cpm_options.chmod && !cpm_options.inode && !cpm_options.umask && !cpm_options.link && !cpm_options.truncate) makeSparse(cpm_options);
    
    // TODO Nezabudnut dalsie kontroly kombinacii prepinacov ...
    
    //-------------------------------------------------------------------
    // Kopirovanie suborov
    //-------------------------------------------------------------------
    
    // TODO Implementovat kopirovanie suborov
/*    if (cpm_options.fast) fast_copy(cpm_options);
    else if (cpm_options.slow) slow_copy(cpm_options);
    else copy(cpm_options);*/
    copy(cpm_options);

    //--truncate je pritomny a kopirovanie bolo uspesne
    if (cpm_options.truncate) trun(cpm_options);

    
    //-------------------------------------------------------------------
    // Vypis adresara
    //-------------------------------------------------------------------
    
    if (cpm_options.directory) {
        printDirectory(cpm_options);
    }
        
    //-------------------------------------------------------------------
    // Osetrenie prepinacov po kopirovani
    //-------------------------------------------------------------------
    
    // TODO Implementovat osetrenie prepinacov po kopirovani
    
    return 0;
}

void makeSparse(struct CopymasterOptions cpm_options){
    int infile_fd = open(cpm_options.infile, O_RDONLY);
    if (infile_fd == -1) FatalError(83, "INA CHYBA\n", 41);

    int outfile_fd = open(cpm_options.outfile, O_WRONLY | O_CREAT);
    if (outfile_fd == -1) FatalError(83, "RIEDKY SUBOR NEVYTVORENY\n", 41);

    struct stat stat_in;
    stat(cpm_options.infile, &stat_in);

    struct stat stat_out;
    stat(cpm_options.outfile, &stat_out);

    stat_out.st_blksize = stat_in.st_blksize;

    char buf[FAST];

    while (read(infile_fd, &buf, FAST) > 0){
        write(outfile_fd, &buf, strlen(buf));
    }

    ftruncate(outfile_fd, stat_in.st_size);

    close(infile_fd);
    close(outfile_fd);

    exit(EXIT_SUCCESS);

}

void fast_copy(struct CopymasterOptions cpm_options){
    int infile_fd = open(cpm_options.infile, O_RDONLY);

    if (infile_fd == -1) FatalError(102, "INA CHYBA\n", 21);

    int outfile_fd = open(cpm_options.outfile, O_WRONLY|O_CREAT);
    if (outfile_fd == -1) FatalError(102, "INA CHYBA\n", 21);

    char buf[FAST] = {0};
//    int count = 0;

    //read and write from and to the files
    while (read(infile_fd, &buf, FAST) > 0){
        write(outfile_fd, &buf, strlen(buf));
     //   printf("Write cycle %d\n", count++);
    }


    close(infile_fd);
    close(outfile_fd);
}


void slow_copy(struct CopymasterOptions cpm_options){
    int infile_fd = open(cpm_options.infile, O_RDONLY);

    if (infile_fd == -1) FatalError(102, "INA CHYBA\n", 21);

    int outfile_fd = open(cpm_options.outfile, O_WRONLY|O_CREAT);

    char buf[2] = {0};
//    int count = 0;

    //read and write from and to the files
    while (read(infile_fd, &buf, 1) > 0){
        write(outfile_fd, &buf, strlen(buf));
      //  printf("Write cycle %d\n", count++);
    }
 

    close(infile_fd);
    close(outfile_fd);
}


void copy(struct CopymasterOptions cpm_options){
    //--link parameter is present and inode too
    if (cpm_options.link && cpm_options.inode){
        checkInode(cpm_options);
        makeLink(cpm_options);
    }
    //only link is present
    else if (cpm_options.link) makeLink(cpm_options);
    

    //OPEN INFILE
    int infile_fd = open(cpm_options.infile, O_RDONLY);

    //PRINT ERROR MESSAGE WHEN ERROR OCCURED
    if (infile_fd == -1){
        switch (errno){
            case 2: FatalError(66, "SUBOR NEEXISTUJE\n", 21);
                    break;
            default: FatalError(66, "INA CHYBA\n", 21);
        }
    }

    int outfile_fd;

    //now we have checked that the infile exists
    //if -i parameter is present, check if the inode and the selected number are the same, if not, throw an error, otherwise pass
    if (cpm_options.inode) checkInode(cpm_options);


    //--create argument IS PRESENT
    if (cpm_options.create == 1){
        //INVALID PERMISSIONS FOR NEW FILE
        if (cpm_options.create_mode == 0) FatalError(99, "ZLE PRAVA\n", 23);

        //TODO UMASK
        if (cpm_options.umask){
            //mode_t mask;
            mode_t mask = umask(0);
            mask = cpm_options.create_mode;

            umask(0026);

            for (int i = 0; i < 4; i++){
                //if (cpm_options.umask_options[i][0] && cpm_options.umask_options[i][])
                switch (cpm_options.umask_options[i][0]){
                    //for user group "OTHERS"
                    case 'o':
                        switch (cpm_options.umask_options[i][2]){
                            case 'r':
                                if (!S_IROTH) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 4 : mask - 4;
                                break;
                            case 'w':
                                if (!S_IWOTH) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 2 : mask - 2;
                                break;
                            case 'x':
                                if (!S_IXOTH) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 1 : mask - 1;
                                break;
                            default: 
                                break;
                        }
                        break;
                    //for user group "GROUP"
                    case 'g':
                        switch (cpm_options.umask_options[i][2]){
                            case 'r':
                                if (!S_IRGRP) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 40 : mask - 40;
                                break;
                            case 'w':
                                if (!S_IWGRP) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 20 : mask - 20;
                                break;
                            case 'x':
                                if (!S_IXGRP) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 10 : mask - 10;
                                break;
                            default:
                                break;
                        }
                        break;
                    //for user group "USER"
                    case 'u':
                        switch (cpm_options.umask_options[i][2]){
                            case 'r':
                                if (!S_IRUSR) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 400 : mask - 400;
                                break;
                            case 'w':
                                if (!S_IWUSR) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 200 : mask - 200;
                                break;
                            case 'x':
                                if (!S_IXUSR) mask = (cpm_options.umask_options[i][1] == '+') ? mask + 100 : mask - 100;
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        FatalError('u', "ZLA MASKA\n", 32);
                        break;
                }

            }
            outfile_fd = open(cpm_options.outfile, O_EXCL|O_CREAT|O_WRONLY, mask);
        }

        else {
            //open outfile
            outfile_fd = open(cpm_options.outfile, O_EXCL|O_CREAT|O_WRONLY, cpm_options.create_mode);
            //ERROR OCCURED UPON CREATING OUTFILE
            if (outfile_fd == -1){
                switch (errno){
                    case 17: FatalError(99, "SUBOR EXISTUJE\n", 23);
                             break;
                    default: FatalError(99, "INA CHYBA\n", 23);
                             break;
                }
            }
        }
    }
    //--overwrite IS PRESENT
    else if (cpm_options.overwrite == 1){
        outfile_fd = open(cpm_options.outfile, O_WRONLY);
        if (outfile_fd == -1){
            switch (errno){
                case 2: FatalError(111, "SUBOR NEEXISTUJE\n", 24);
                        break;
                default: FatalError(111, "INA CHYBA\n", 24);
                         break;
            }
        }
    }
    //--append IS PRESENT
    else if (cpm_options.append == 1){
        outfile_fd = open(cpm_options.outfile, O_WRONLY|O_APPEND);
        if (outfile_fd == -1){
            switch (errno){
                case 2: FatalError(97, "SUBOR NEEXISTUJE\n", 22);
                        break;
                default: FatalError(97, "INA CHYBA\n", 22);
                         break;
            }
        }
    }

    //--create, --overwrite, --append IS NOT PRESENT
    else{
        //OPEN OUTFILE
        outfile_fd = open(cpm_options.outfile, O_WRONLY);
        //OUTFILE DOESN'T HAVE WRITE PERMISSIONS
        if (outfile_fd == -1 && errno == EACCES) FatalError(66, "INA CHYBA\n", 21);

        //OUTFILE DOESN'T EXIST
        if (outfile_fd == -1){
            //get the permissions of input file
            struct stat statbuf;
            fstat(infile_fd, &statbuf);

            outfile_fd = open(cpm_options.outfile, O_CREAT|O_WRONLY);
            //set the permissions of the newly created file
            fchmod(outfile_fd, statbuf.st_mode);
        }
    }
    
    //set a variable for number of bytes read. If lseek is used, set this variable to the number of bytes to read, otherwise use preset macros
    int bytes_to_read;

    //--lseek IS PRESENT
    if (cpm_options.lseek == 1){
        if (cpm_options.lseek_options.pos1 < 0) FatalError(108, "CHYBA POZICIE infile\n", 33);
        if (cpm_options.lseek_options.pos2 < 0) FatalError(108, "CHYBA POZICIE outfile\n", 33);
//        if (cpm_options.lseek_options.num < 0) FatalError(108, "INA CHYBA\n", 33);

        switch (cpm_options.lseek_options.x){
            case 0: lseek(outfile_fd, cpm_options.lseek_options.pos2, SEEK_SET);
                    break;
            case 2: lseek(outfile_fd, cpm_options.lseek_options.pos2, SEEK_END);
                    break;
            case 1: lseek(outfile_fd, cpm_options.lseek_options.pos2, SEEK_CUR);
                    break;
            default: FatalError(108, "INA CHYBA\n", 33);
                     break;
                     
         }
        lseek(infile_fd, cpm_options.lseek_options.pos1, SEEK_SET);

        bytes_to_read = cpm_options.lseek_options.num;

        char buf[bytes_to_read + 1];
        memset(buf, 0, (bytes_to_read + 1) * sizeof(int));
        read(infile_fd, &buf, bytes_to_read);
        write(outfile_fd, &buf, strlen(buf));
        exit(EXIT_SUCCESS);
    }


    //int count = 1;
    //FAST OPTION
    if (cpm_options.fast == 1){
        char buf[FAST] = {0};
        bytes_to_read = FAST;

        //TODO
        if (cpm_options.lseek){
            read(infile_fd, &buf, bytes_to_read);
            write(outfile_fd, &buf, strlen(buf));
        }
        
        else {
            //read and write from and to the files
            while (read(infile_fd, &buf, FAST) > 0){
                write(outfile_fd, &buf, strlen(buf));
              //  printf("Write cycle %d\n", count++);
            }
        }
    }

    //SLOW OPTION
    else if (cpm_options.slow == 1){
        char buf[2] = {0};

        if (cpm_options.lseek){
            while (read(infile_fd, &buf, 1) > 0 || bytes_to_read > 0){
                write(outfile_fd, &buf, 1);
                bytes_to_read--;
            }
        }

        else {
            //read and write from and to the files
            while (read(infile_fd, &buf, 1) > 0){
                write(outfile_fd, &buf, strlen(buf));
            //    printf("Write cycle %d\n", count++);
            }
        }
    }
    //NO OPTION
    else {
        char buf[NORMAL] = {0};

        if (cpm_options.lseek){
            while (read(infile_fd, &buf, 100) > 0 || bytes_to_read > 0){
                write(outfile_fd, &buf, 100);
                bytes_to_read-=100;
            }
        }

        else {
            //read and write 100 characters from the infile to outfile until there are no more chars to copy
            while (read(infile_fd, &buf, NORMAL) > 0){
                write(outfile_fd, &buf, strlen(buf));
            }
        }
    }


    //DELETE FILE
    if (cpm_options.delete_opt){
        //get the file properties
        struct stat infile_stat;
        stat(cpm_options.infile, &infile_stat);
        //check if the file is a regular file and if so, remove it
        if (S_ISREG(infile_stat.st_mode)){
            //close the file before removing it
            close(infile_fd);
            //removal of the file
            int remove = unlink(cpm_options.infile);
            //the program was unable to remove file because of permissions
            if (remove == -1 && errno == EACCES) FatalError(100, "SUBOR NEBOL ZMAZANY\n", 26);
            //any other error with removing the file
            else if (remove == -1) FatalError(100, "INA CHYBA\n", 26);
        }
    }
    //DON'T DELETE FILE
    else {
        //close files
        close(outfile_fd);
        close(infile_fd);
    }


    //CHMOD --> change permissions of the outfile after it is created (and closed)
    if (cpm_options.chmod){
        setPermissions(cpm_options);
    }

}


void setPermissions(struct CopymasterOptions cpm_options){
    if (cpm_options.chmod_mode == 0) FatalError(109, "ZLE PRAVA\n", 34);

    //mode_t chmod_mode
    int setPermission = chmod(cpm_options.outfile, cpm_options.chmod_mode);

    if (setPermission == -1) FatalError(109, "INA CHYBA\n", 34);

}

void checkInode(struct CopymasterOptions cpm_options){
    struct stat infile_stat;
    stat(cpm_options.infile, &infile_stat);

    if (infile_stat.st_ino != cpm_options.inode_number) FatalError(105, "INY INODE\n", 27);
    else if (!S_ISREG(infile_stat.st_mode)) FatalError(105, "ZLY TYP VSTUPNEHO SUBORU\n", 27);

}


void printDirectory(struct CopymasterOptions cpm_options){
    //open directory
    DIR* openDirectory = opendir(cpm_options.infile);
    //unsuccessful openning, file is not a directory
    if (openDirectory == NULL) FatalError(68, "VSTUPNY SUBOR NIE JE ADRESAR\n", 28);

    //dirent structure where we will save entries from readdir
    struct dirent *directoryEntry;
    //stat structure where we will save file information from the file name we got from directoryEntry.d_name
    struct stat dirStat;

    //open outfile
    int outfile_fd = open(cpm_options.outfile, O_WRONLY);
    //error occured upon opening outfile
    if (outfile_fd == -1) FatalError(68, "VYSTUPNY SUBOR-CHYBA\n", 28);



    char pathname[266] = {0};
    //get directory entries
    while ((directoryEntry = readdir(openDirectory)) != NULL){
        //here I ran into a problem and then I found out that the error was that I was not providing the correct pathname, only the name of the file, but not the pathname, writing this comment here to remind myself of this
        pathname[0] = 0;
        strcat(pathname, cpm_options.infile);
        strcat(pathname, "/");
        strcat(pathname, directoryEntry->d_name);

        stat(pathname, &dirStat);
        //use ternary because we haven't used in in quite the time and also it's easier this way and it works
        //is there any easier way to do this???
        write(outfile_fd, S_ISDIR(dirStat.st_mode) ? "d" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IRUSR) ? "r" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IWUSR) ? "w" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IXUSR) ? "x" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IRGRP) ? "r" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IWGRP) ? "w" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IXGRP) ? "x" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IROTH) ? "r" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IWOTH) ? "w" : "-", 1);
        write(outfile_fd, (dirStat.st_mode & S_IXOTH) ? "x" : "-", 1);
        write(outfile_fd, " ", 1);

        //now write number of links
        char buf[2];
        //we have to use sprintf to format the string, otherwise we get passing argument 2 of 'write' makes pointer from integer without a cast and it wants to get const void *__buf and we have int
        sprintf(buf, "%ld", dirStat.st_nlink);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);

        //write UID and GID
        sprintf(buf, "%d", dirStat.st_uid);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);
        
        sprintf(buf, "%d", dirStat.st_gid);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);

        //size
        sprintf(buf, "%ld", dirStat.st_size);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);

        //last modification

        //use struct tm from time.h
        //for more info go to man gmtime and man struct timespec
        //TODO check man for strftime, may be of interest to us
        struct tm last_mod = *gmtime(&dirStat.st_mtime);
        //day
        sprintf(buf, "%d", last_mod.tm_mday);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, "-", 1);
        //month
        sprintf(buf, "%d", last_mod.tm_mon + 1);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, "-", 1);
        //year
        sprintf(buf, "%d", last_mod.tm_year + 1900);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);


 /*       sprintf(buf, "%ld", dirStat.st_mtim.tv_sec);
        write(outfile_fd, buf, strlen(buf));
        write(outfile_fd, " ", 1);*/

        write(outfile_fd, directoryEntry->d_name, strlen(directoryEntry->d_name));
        write(outfile_fd, "\n", 1);
    }

    close(outfile_fd);
    closedir(openDirectory);

}

void makeLink(struct CopymasterOptions cpm_options){
    //open infile
    int infile_fd = open(cpm_options.infile, O_RDONLY);
    //infile doesn't exist, error
    if (infile_fd == -1){
        switch (errno){
            case 2: FatalError(75, "VSTUPNY SUBOR NEEXISTUJE\n", 30);
                    break;
            default: FatalError(75, "INA CHYBA\n", 30);
                     break;
        }
    }
    //check if outfile exists, if so, print to stderr and exit program
    int outfile_fd = open(cpm_options.outfile, O_EXCL|O_CREAT);
    //outfile does exist, error
    if (outfile_fd == -1){
        switch (errno){
            case 17: FatalError(75, "VYSTUPNY SUBOR UZ EXISTUJE\n", 30);
                     break;
            default: break;//FatalError(75, "INA CHYBA\n", 30);
        }
    }
    close(outfile_fd);
    //we have to remove the outfile creating in the process of checking if the outfile already exists because otherwise it throws us file exists errno
    unlink(cpm_options.outfile);
    close(infile_fd);

    link(cpm_options.infile, cpm_options.outfile) == 0 ? exit(EXIT_SUCCESS) : FatalError(75, "INA CHYBA\n", 30);

   /* if (link(cpm_options.infile, cpm_options.outfile) == 0) exit(EXIT_SUCCESS);
    else FatalError(75, "INA CHYBA 3\n", 30);*/
    //Ak tomu rozumiem správne, pri tejto funkcii obsah nekopírujeme, iba vytvoríme hard link, preto program ukončíme
    //exit(EXIT_SUCCESS);

}

void trun(struct CopymasterOptions cpm_options){
    //bola uvedena zaporna size
    if (cpm_options.truncate_size < 0) FatalError(116, "ZAPORNA VELKOST\n", 31);
    //zavolame sluzbu truncate
    int truncat = truncate(cpm_options.infile, cpm_options.truncate_size);
    //truncate bolo neuspesne
    if (truncat == -1) FatalError(116, "INA CHYBA\n", 31);
}

void FatalError(char c, const char* msg, int exit_status)
{
   /* fprintf(stderr, "%c:%d\n", c, errno); 
    fprintf(stderr, "%c:%s\n", c, strerror(errno));
    fprintf(stderr, "%c:%s\n", c, msg);
    exit(exit_status);
    */
    fprintf(stderr, "%c:%d:%s:%s\n", c, errno, strerror(errno), msg);
    exit(exit_status);
}

void PrintCopymasterOptions(struct CopymasterOptions* cpm_options)
{
    if (cpm_options == 0)
        return;
    
    printf("infile:        %s\n", cpm_options->infile);
    printf("outfile:       %s\n", cpm_options->outfile);
    
    printf("fast:          %d\n", cpm_options->fast);
    printf("slow:          %d\n", cpm_options->slow);
    printf("create:        %d\n", cpm_options->create);
    printf("create_mode:   %o\n", (unsigned int)cpm_options->create_mode);
    printf("overwrite:     %d\n", cpm_options->overwrite);
    printf("append:        %d\n", cpm_options->append);
    printf("lseek:         %d\n", cpm_options->lseek);
    
    printf("lseek_options.x:    %d\n", cpm_options->lseek_options.x);
    printf("lseek_options.pos1: %ld\n", cpm_options->lseek_options.pos1);
    printf("lseek_options.pos2: %ld\n", cpm_options->lseek_options.pos2);
    printf("lseek_options.num:  %lu\n", cpm_options->lseek_options.num);
    
    printf("directory:     %d\n", cpm_options->directory);
    printf("delete_opt:    %d\n", cpm_options->delete_opt);
    printf("chmod:         %d\n", cpm_options->chmod);
    printf("chmod_mode:    %o\n", (unsigned int)cpm_options->chmod_mode);
    printf("inode:         %d\n", cpm_options->inode);
    printf("inode_number:  %lu\n", cpm_options->inode_number);
    
    printf("umask:\t%d\n", cpm_options->umask);
    for(unsigned int i=0; i<kUMASK_OPTIONS_MAX_SZ; ++i) {
        if (cpm_options->umask_options[i][0] == 0) {
            // dosli sme na koniec zoznamu nastaveni umask
            break;
        }
        printf("umask_options[%u]: %s\n", i, cpm_options->umask_options[i]);
    }
    
    printf("link:          %d\n", cpm_options->link);
    printf("truncate:      %d\n", cpm_options->truncate);
    printf("truncate_size: %ld\n", cpm_options->truncate_size);
    printf("sparse:        %d\n", cpm_options->sparse);
}
