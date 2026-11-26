// Hluboke vnoreni bloku a odsazeni.
import ifj26.*

@main def nestedBlocksTest(): Unit =
  var x = 0
  if x == 0 then
    while x < 3 do
      if x == 2 then
        print("two\n")
      else
        print("less\n")
      x = x + 1
  else
    print("never\n")
  print("done\n")
