/* © 2018 David Given
 * This program is distributable under the terms of the 2-clause BSD license. See:
 * https://opensource.org/licenses/BSD-2-Clause
 * ...for the full text.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <linux/fs.h>

static const char* filename = "broken.avi";
static const char* jpeg_pattern = "jpegs/%05d.jpg";
static int desired_width = 720;
static int desired_height = 240;
static uint64_t start_offset = 0;

static void error(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "error: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static char* aprintf(const char* s, ...)
{
    va_list ap;
    va_start(ap, s);
    size_t needed = vsnprintf(NULL, 0, s, ap) + 1;
    char* buffer = malloc(needed);
	va_end(ap);

    va_start(ap, s);
    vsprintf(buffer, s, ap);
	va_end(ap);
    return buffer;
}

static void parse_options(int argc, char* const* argv)
{
    for (;;)
    {
        switch (getopt(argc, argv, "f:j:w:h:o:"))
        {
            case -1:  return;
            case 'f': filename = optarg; break;
            case 'j': jpeg_pattern = optarg; break;
            case 'w': desired_width = strtoull(optarg, NULL, 0); break;
            case 'h': desired_height = strtoull(optarg, NULL, 0); break;
            case 'o': start_offset = strtoull(optarg, NULL, 0); break;

            default:
                error("syntax: avifixer -f <inputfile> -j <output/%d.jpg> [-w width] [-h height] [-o startoffset]");
        }
    }
}

int main(int argc, char* const* argv)
{
    parse_options(argc, argv);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        error("couldn't open input file: %s", strerror(errno));

    struct stat st;
    if (ioctl(fd,BLKGETSIZE64, &st.st_size) == -1)
        fstat(fd, &st);

    printf("scanning 0x%lx byte file: %s\n", st.st_size, filename);

    const uint8_t* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, fd, 0);
    if (data == MAP_FAILED)
        error("couldn't map file");

    int count = 0;
    uint64_t lastheader = ULONG_MAX;
    bool sized = false;
    for (uint64_t offset = start_offset; offset < (st.st_size-2); offset++)
    {
        if ((data[offset+0] == 0xff) && (data[offset+1] == 0xd8))
        {
            printf("found header at 0x%lx (%d%%)\n", offset,
				offset * 100 / st.st_size);
            lastheader = offset;
            sized = false;
        }

        if (lastheader != ULONG_MAX)
        {
            if ((data[offset+0] == 0xff) && (data[offset+1] == 0xc0))
            {
                int height = (data[offset+5]<<8) | data[offset+6];
                int width = (data[offset+7]<<8) | data[offset+8];
                if ((desired_width == 0) || ((width == desired_width) && (height == desired_height)))
                    sized = true;
                else
                {
                    printf("image size of %d x %d does not match, skipping\n", width, height);
                    lastheader = ULONG_MAX;
                }
            }

            if ((data[offset+0] == 0xff) && (data[offset+1] == 0xd9) && sized)
            {
                printf("found footer at 0x%lx\n", offset);

                char* outputfilename = aprintf(jpeg_pattern, count);
                printf("saving to %s\n", outputfilename);

                int outputfd = open(outputfilename, O_CREAT|O_WRONLY, 0666);
                if (outputfd == -1)
                    error("couldn't open output file: %s", strerror(errno));
                if (write(outputfd, &data[lastheader], offset+2-lastheader) <= 0)
                    error("short write: %s", strerror(errno));
                close(outputfd);

                count++;
                lastheader = ULONG_MAX;
                free(outputfilename);
            }
        }
    }
    exit(1);
}
