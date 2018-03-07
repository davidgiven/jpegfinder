Jpegfinder
==========

What?
-----

Jpegfinder is a tiny tool for scraping through files (or block devices) and
finding and extracting anything which looks a bit like a JPEG image.

Why?
----

I have a Hubsan H107D drone, which records video onto an SD card; but it's
buggy and the resulting AVI MJPEG files are hopelessly corrupt. Jpegfinder
will extract any (surviving) frames of video from the files, and then I can
turn them into playable videos.

It's also useful for extracting interesting looking jpegs from any binary
file. I suppose technically it's a forensics tool.

How?
----

Compile the tool (by running make). Invoke with:

```
$ jpegfinder -f broken.avi -j outputdir -w 720 -h 240
```

Command line flags are:

  * <code>-f filename.avi</code>: specifies the input file.

  * <code>-j outputdir</code>: specifies the directory output files go.

  * <code>-w width -h height</code>: if set to anything other than 0, then only
    jpegs of this size will be output. This is useful for reducing false
    positives when you know the size of the images you're looking for (like I
    do for my videos). (If not set, jpegfinder will emit everything, including
    garbage.)

  * <code>-o offset</code>: start looking at this offset. Useful for resuming
    a scan (because scans are slow).

Output files are named by the hex offset into the source file. This is so each
file has a stable name, allowing runs to be repeated or resumed and you get
the same names each time.

You can create a non-corrupt MJPEG video containing the output frames using
ffmpeg, like this:

```
$ ffmpeg -f image2 -pattern_type glob -framerate 60 -i 'jpegs/*.jpg' -vcodec copy output.mkv
```

Note that if jpegfinder found any false positives, you'll get corrupt frames.
But any competent video editor should find and ignore them. At least the file
will be loadable (unlike the source).

Who?
----

jpegfinder was written entirely by me, David Given. Feel free to send me
email at [dg@cowlark.com](mailto:dg@cowlark.com). You may also [like to visit
my website](http://cowlark.com); there may or may not be something
interesting there.

License?
--------

jpefinder is open source software available [under the 2-clause BSD
license](https://github.com/davidgiven/jpegfinder/blob/master/LICENSE).
Simplified summary: do what you like with it, just don't claim you wrote it.
