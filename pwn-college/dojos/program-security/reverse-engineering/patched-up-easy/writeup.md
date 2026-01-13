





Notes:

Expected result: d9 e3 1e 04 a6 18 43 b7 99 68 a8 ad c4 be f3 be 00 00 00 00 00 00 00 00 00 00

```
hacker@reverse-engineering~patched-up-easy:/challenge$ ./patched-up-easy
###
### Welcome to ./patched-up-easy!
###

This license verifier software will allow you to read the flag. However, before you can do so, you must verify that you
are licensed to read flag files! This program consumes a license key over stdin. Each program may perform entirely
different operations on that input! You must figure out (by reverse engineering this program) what that license key is.
Providing the correct license key will net you the flag!

Unfortunately for you, the license key cannot be reversed. You'll have to crack this program.

Changing byte 1/5.
Offset (hex) to change: 100
New value (hex): 90
The byte has been changed: *0x654ac203c100 = 90.
Changing byte 2/5.
Offset (hex) to change: 100
New value (hex): 90
The byte has been changed: *0x654ac203c100 = 90.
Changing byte 3/5.
Offset (hex) to change: 100
New value (hex): 90
The byte has been changed: *0x654ac203c100 = 90.
Changing byte 4/5.
Offset (hex) to change: 100
New value (hex): 90
The byte has been changed: *0x654ac203c100 = 90.
Changing byte 5/5.
Offset (hex) to change: 100
New value (hex): 90
The byte has been changed: *0x654ac203c100 = 90.
Ready to receive your license key!

12345
Initial input:

        31 32 33 34 35 0a 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

This challenge is now mangling your input using the `md5` mangler. This mangler cannot be reversed.

This mangled your input, resulting in:

        9c 87 3b e9 62 78 58 73 ae a4 81 e3 0b 8d 4a 73 00 00 00 00 00 00 00 00 00 00

The mangling is done! The resulting bytes will be used for the final comparison.

Final result of mangling input:

        9c 87 3b e9 62 78 58 73 ae a4 81 e3 0b 8d 4a 73 00 00 00 00 00 00 00 00 00 00

Expected result:

        d9 e3 1e 04 a6 18 43 b7 99 68 a8 ad c4 be f3 be 00 00 00 00 00 00 00 00 00 00

Checking the received license key!

Wrong! No flag for you!
```

1BCC
1BCD
