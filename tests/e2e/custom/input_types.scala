// Cteni vstupu: neplatny vstup readInt vraci 0, readDouble, readBoolean.
import ifj26.*

@main def inputTypesTest(): Unit =
  val bad = readInt()
  print(bad)
  print("\n")
  val d = readDouble()
  val di = toInt(d)
  print(di)
  print("\n")
  val b = readBoolean()
  if b then
    print("yes\n")
  else
    print("no\n")
