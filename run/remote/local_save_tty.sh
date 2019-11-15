#!/bin/sh

# Command to save terget's com port output to local file.

# My config uses COM to TCP convertor connected to target's
# com port. This command saves output from convertor to file
nc etherwan. 601 >boot.log
