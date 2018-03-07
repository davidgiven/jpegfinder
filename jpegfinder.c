/* © 2018 David Given
 * This program is distributable under the terms of the 2-clause BSD license.
 * See the LICENSE file in this directory for the full text.
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

#if defined __linux__
#include <linux/fs.h>
#endif

static const char* filename = "broken.avi";
static const char* jpeg_dir = "jpegs";
static int desired_width = 0;
static int desired_height = 0;
static uint64_t start_offset = 0;

static const char FILENAME_PATTERN[] = "%s/%016lx.jpg";

static const uint8_t* data;
static uint64_t length;

static uint8_t byteat(uint64_t offset)
{
    if (offset < length)
        return data[offset];
    return 0;
}

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
            case 'j': jpeg_dir = optarg; break;
            case 'w': desired_width = strtoull(optarg, NULL, 0); break;
            case 'h': desired_height = strtoull(optarg, NULL, 0); break;
            case 'o': start_offset = strtoull(optarg, NULL, 0); break;

            default:
                error("syntax: avifixer -f <inputfile> -j <outputdir> [-w width] [-h height] [-o startoffset]");
        }
    }
}

int main(int argc, char* const* argv)
{
    parse_options(argc, argv);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        error("couldn't open input file: %s", strerror(errno));

    {
        struct stat st;
        #if defined __linux__
            if (ioctl(fd, BLKGETSIZE64, &st.st_size) == -1)
                fstat(fd, &st);
        #else
            fstat(fd, &st);
        #endif
        length = st.st_size;
    }

    printf("scanning 0x%lx byte file: %s\n", length, filename);

    data = mmap(NULL, length, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, fd, 0);
    if (data == MAP_FAILED)
        error("couldn't map file");

    int count = 0;
    uint64_t lastheader = ULONG_MAX;
    bool sized = false;
    for (uint64_t offset = start_offset; offset != length; offset++)
    {
        if ((offset & 0xfffff) == 0)
            printf("progress: 0x%lx / 0x%lx (%d%%)\n", offset, length, offset*100 / length);

        if ((byteat(offset+0) == 0xff) && (byteat(offset+1) == 0xd8))
        {
            lastheader = offset;
            sized = false;
        }

        if (lastheader != ULONG_MAX)
        {
            if ((byteat(offset+0) == 0xff) && (byteat(offset+1) == 0xc0))
            {
                int height = (byteat(offset+5)<<8) | byteat(offset+6);
                int width = (byteat(offset+7)<<8) | byteat(offset+8);
                if ((desired_width == 0) || ((width == desired_width) && (height == desired_height)))
                    sized = true;
                else
                    lastheader = ULONG_MAX;
            }

            if ((byteat(offset+0) == 0xff) && (byteat(offset+1) == 0xd9) && sized)
            {
                char* outputfilename = aprintf(FILENAME_PATTERN, jpeg_dir, offset);
                printf("found image: %s\n", outputfilename);

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
