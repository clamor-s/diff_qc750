grabtrace

grabtrace is a python script used to pull tracing data from a kernel built
with the tracedump module (CONFIG_TRACEDUMP=y). It is known to work properly
with python version 2.6.6

tracedump will dump ftrace data in one of two forms: ASCII or binary. The
binary form is used by grabtrace to build a trace.dat file, which can be read
with trace-cmd or kernelshark. ASCII data is human-readable. The dump format
is binary by default, but this can be chanced via the tracedump_ascii module
parameter. To turn on ascii dumping:

echo 1 > /sys/module/tracedump/parameters/tracedump_ascii

Please select the dumping form before running grabtrace. If you select binary
data, you will need trace-cmd or kernelshark to read the data:

sudo apt-get install trace-cmd
sudo apt-get install kernelshark

tracedump can dump data in two places. By default, it will always dump data
during a kernel panic to the console. If ram_console is enabled in the kernel,
this can be retrieved on the next boot by reading from /proc/last_kmsg.
Tracedump can also be pulled manually at any time by reading
from /proc/tracedump.

tracedump data is compressed using zlib. If grabtrace is asked to pull ascii
data, it will simply locate the data by looking for a blob surrounded by
TRACEDUMP .... TRACEDUMP, and then decompress the data.

If tracedump_ascii == 1, you can pull the data using

grabtrace -a -p /proc/tracedump

or

grabtrace -a -p /proc/last_kmsg

Note that tracedump_ascii will reset to 0 on each boot.

If -a is not included, grabtrace will pull additional files from the device
in order to create a trace.dat file, which can be read using trace-cmd or
kernelshark.

If tracedump_ascii == 0, you can pull the data using:

grabtrace -p /proc/tracedump

or

grabtrace -p /proc/last_kmsg

Either of these methods will produce a trace.dat file. In the binary case,
you can read it with using:

trace-cmd report trace.dat

or

kernelshark trace.dat

Search the internet for trace-cmd or kernelshark for more information on
these tools.

For a full list of grabtrace parameters, use:

grabtrace -h
