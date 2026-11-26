// Vestavene funkce pro retezce a retezcove operace.
import ifj26.*

@main def stringBuiltinsTest(): Unit =
  val s = "hello"
  val len = strLength(s)
  val sub = subStr(s, 1, 4)
  val code = ord(s, 0)
  val ch = chr(97)
  val joined = s + " world"
  print(len)
  print("\n")
  print(sub)
  print("\n")
  print(code)
  print("\n")
  print(ch)
  print("\n")
  print(joined)
  print("\n")
  if sub == "ell" then
    print("equal\n")
  else
    print("neq\n")
