# $(realpath ) gives good cywgin path in Windows, and, I believe, is 
# harmless in Unix.

# Directory where to put executables - supposed to be in PATH
INST_BIN=$(realpath c:\bin\tools)
