// Vice prikazu na jednom radku oddelenych strednikem.
import ifj26.*

@main def semicolonsTest(): Unit =
  var x = 1; var y = 2
  x = x + y; y = x + y
  print(x); print("\n")
  print(y)
  print("\n")
