#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

#define kUMASK_OPTIONS_MAX_SZ 10

struct CopymasterOptionsLseek {
    int x;
    off_t pos1;
    off_t pos2;
    size_t num;
};

struct CopymasterOptions {
    const char * infile;
    const char * outfile;
    int fast;
    int slow;
    int create;
    mode_t create_mode;
    int overwrite;
    int append;
    int lseek;
    struct CopymasterOptionsLseek lseek_options;
    int directory;
    int delete_opt;
    int chmod;
    mode_t chmod_mode;
    int inode;
    ino_t inode_number;
    int umask;
    char umask_options[kUMASK_OPTIONS_MAX_SZ][4];
    int link;
    int truncate;
    off_t truncate_size;
    int sparse;
};

struct CopymasterOptions ParseCopymasterOptions(int argc, char *argv[]);

void FatalError(char c, const char* msg, int exit_status);

#endif /* UTIL_H */
