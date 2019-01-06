// mycopy.c 
// Author : Murtaza Meerza
// I collaborated along side Xavier Sepulveda and we exchanged ideas back and forth.

/* mycopy utility
 * makes a copy of a file and assigns the same file
 * permissions to the copy
 * Usage:
 *   ./mycopy <name of original file> <name of copy>
 * If the original file does not exist or the user
 * lacks permission to read it, mycopy emits an error.
 * Also, if a file or directory exists with the name
 * proposed for the copy, mycopy emits an error and
 * terminates.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int error( char * msg )
{
    perror( msg );
    return 2;
}

int usage( char * name )
{
    printf( "Usage: %s <file to copy> <name of copy>\n", name );
    return 1;
}

int main(int argc, char * argv[])
{
    if (argc != 3)
        return usage(argv[0]);

    int readerfile = open(argv[1], O_RDONLY, 0);
    if (readerfile == -1) {
        perror(argv[1]);
        return 1;
    }
    int outputfile = open(argv[2], O_CREAT|O_EXCL| O_RDONLY | O_RDWR);
    if (outputfile == -1) {
        perror(argv[2]);
        return 1;
        }

    struct stat stat_struct;
    if (stat(argv[1], &stat_struct) == -1) {
        perror("fstat file");
        return 1;
    }


    fchmod(outputfile, stat_struct.st_mode);


    // reading bytes from original file
    char buffer[4194304]; // need large buffer to decrease time
    int bytes_read_in;
    do {
        // read one byte at a time
        bytes_read_in = read(readerfile, buffer, 4194304);
        if (bytes_read_in > 0) {
					// writing to copy
                    write(outputfile, &buffer, bytes_read_in);
            }
        } while (bytes_read_in > 0); // zero bytes == EOF

        if (bytes_read_in == -1)
            perror("file read");
		// checks for file reading error


    close(readerfile);
    close(outputfile);

        return 0;
    }
