# PicoCTF: Perplexed

## Context

We are given a binary named "perplexed". We are given no hints.

## Exploitation

Opening the file in Ghidra, we can begin analyzing it. Let's look at the `main` function (some parts truncated for brevity):

``` text
bool main(void)

{
  bool bVar1;
  char local_118 [268];
  int local_c;
  
  local_118[0] = '\0';
  (...zeroes out the array...)
  local_118[0xff] = '\0';
  printf("Enter the password: ");
  fgets(local_118,0x100,stdin);
  local_c = check(local_118);
  bVar1 = local_c != 1;
  if (bVar1) {
    puts("Correct!! :D");
  }
  else {
    puts("Wrong :(");
  }
  return !bVar1;
}
```

We


# Sources/Credits

Written by Madalina Stoicov
