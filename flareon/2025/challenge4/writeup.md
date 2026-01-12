# FLARE-On 2025: Challenge 4

## Context

We are just provided with an file UnholyDragon-150.exe.

## Background Information

## Vulnerability

We begin as always by inspecting the given file. I ran `file` and was met with nothing useful:

```
FLARE-VM 01/11/2026 20:29:57
PS C:\Users\flare\Desktop > file .\UnholyDragon-150.exe
.\UnholyDragon-150.exe: data
```

I also ran `strings` and noticed two groups of non-gibberish words. These strings can be found in `useful-strings.txt` which is in the same directory as this writeup. One group contained functions from popular Windows libraries, like FreeLibrary, GetProcAddress, and LoadLibraryW. The other group had some function names unfamiliar to me. I pasted these strings into an LLM to see if it could help decipher them, and it said this is indicative of a Windows GUI executable built with Virtual Basic 6. I kept this in mind as I continued inspecting the file.

## Exploitation

## Remediation

# Sources/Credits

Written by Madalina Stoicov

